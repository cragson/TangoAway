#include "parser.hpp"

#include "add-vdr-imm.hpp"
#include "add-vdr-vdr.hpp"
#include "and-vdr-imm.hpp"
#include "and-vdr-vdr.hpp"
#include "cmp.hpp"
#include "dec-vdr.hpp"
#include "div-vdr-imm.hpp"
#include "div-vdr-vdr.hpp"
#include "dump.hpp"
#include "find_signature.hpp"
#include "get-image-base.hpp"
#include "get-image-size.hpp"
#include "if.hpp"
#include "inc-vdr.hpp"
#include "inject_dll.hpp"
#include "jmp-imm.hpp"
#include "logging.hpp"
#include "mov-vdr-imm.hpp"
#include "mov-vdr-vdr.hpp"
#include "mul-vdr-imm.hpp"
#include "mul_vdr_vdr.hpp"
#include "or-vdr-imm.hpp"
#include "or-vdr-vdr.hpp"
#include "patch_bytes.hpp"
#include "println.hpp"
#include "read-mem-imm.hpp"
#include "read-mem-vdr.hpp"
#include "resume_process.hpp"
#include "rpcae.hpp"
#include "show_images_info.hpp"
#include "show_threads_info.hpp"
#include "sleep_ms-imm.hpp"
#include "sleep_ms-vdr.hpp"
#include "sub-vdr-imm.hpp"
#include "sub-vdr-vdr.hpp"
#include "suspend_process.hpp"
#include "terminate_process.hpp"
#include "utils.hpp"
#include "write-mem.hpp"
#include "xor-vdr-imm.hpp"
#include "xor-vdr-vdr.hpp"
#include "je-imm.hpp"
#include "jg-imm.hpp"
#include "jge-imm.hpp"
#include "jl-imm.hpp"
#include "jle-imm.hpp"
#include "jne-imm.hpp"

#include <execution>
#include <future>

parser::parser()
{
	this->m_instructions = {};
}

// <mnemonic> <vdr>
EStatusCode parser::is_valid_two_part_instruction( const std::string& inst )
{
	if( inst.empty() )
		return PARSER_UNKNOWN_INSTRUCTION_FOUND;

	Log::debug( "[parser::is_valid_two_part_instruction] Validating instruction: %s\n", inst.c_str() );

	const auto split_line = Utils::split_string( inst );

	// make sure size is 2 because it should take only one argument
	if( split_line.size() != 2 )
	{
		Log::debug( "[parser::is_valid_two_part_instruction] Detected invalid instruction size: %s\n", inst.c_str() );

		return PARSER_INVALID_INSTRUCTION_SYNTAX;
	}

	const auto& vdr = split_line.back();

	if( !is_vdr( vdr ) )
	{
		Log::debug( "[parser::is_valid_two_part_instruction] Detected a invalid vdr register: %s\n", vdr.c_str() );

		return PARSER_INVALID_INSTRUCTION_VDR;
	}

	Log::debug( "[parser::is_valid_two_part_instruction] Extracted vdr from instruction: %s\n", vdr.c_str() );

	Log::debug( "[parser::is_valid_two_part_instruction] %s is a valid instruction.\n", inst.c_str() );

	return PARSER_SUCCESS;
}

EStatusCode parser::parse_file_multithreaded_chunking( const std::string& path_to_file, const size_t thread_count )
{
	// Check if the path exists
	if( !Utils::does_file_exists( path_to_file ) )
		return PARSER_FILE_PATH_DOES_NOT_EXIST;

	Log::info( "[parser::parse_file_multithreaded] Parsing now %s\n", path_to_file.c_str() );

	// Read content from code file
	auto code = Utils::read_file( path_to_file );

	// If code file is empty, no need to process it
	if( code.empty() )
		return PARSER_PRP_FILE_IS_EMPTY;

	//std::sort(std::execution::par, code.begin(), code.end());

	const auto chunk_size     = code.size() / thread_count;
	const auto chunk_leftover = code.size() % thread_count;

	std::vector< std::shared_ptr< instruction > > th_insts;

	std::mutex mtx;

	size_t current_id = 0;

	const auto parse_code = [&](const size_t start, const size_t end, const size_t id)
	{
		std::vector< std::shared_ptr< instruction > > insts;

		// Check syntax and semantics
		for (size_t idx = start; idx < end; idx++)
		{
			auto& line = code.at(idx);

			// skip empty lines
			if (line.empty())
				continue;

			// make line lowercase
			Utils::string_to_lower(line);

			// split line into multiple strings
			auto split_line = Utils::split_string(line);

			// extract mnemonic from line
			const auto& mnemonic = split_line.front();
			
			// syntax: rpcae
			if( mnemonic == "rpcae" )
			{
				insts.push_back( std::make_shared< inst_rpcae >() );

				continue;
			}

			// syntax: inc <destination>
			// destination := vdr
			if( mnemonic == "inc")
			{
				if( const auto ret = is_valid_two_part_instruction( line ); ret == PARSER_SUCCESS )
				{
					insts.push_back( std::make_shared< inst_inc_vdr >( vdr::vdr_str_to_enum( split_line.back() ) ) );
				}
				else
					return ret;

				continue;
			}

			// syntax: dec <destination>
			// destination := vdr
			if( mnemonic == "dec" )
			{
				if( const auto ret = is_valid_two_part_instruction( line ); ret == PARSER_SUCCESS )
				{
					insts.push_back( std::make_shared< inst_dec_vdr >( vdr::vdr_str_to_enum( split_line.back() ) ) );
				}
				else
					return ret;

				continue;
			}

			// syntax: add <datatype> <destination> <source>
			// datatype := byte, word, dword, qword, float
			// destination := vdr
			// source := vdr, imm
			if( mnemonic == "add" )
			{
				// make sure it has the correct instruction size
				if( split_line.size() != 4 )
				{
					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				// validate the datatype
				const auto& datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) )
				{
					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}

				// validate the destination
				const auto& dest = split_line.at( 2 );

				if( !is_vdr( dest ) )
				{
					return PARSER_INVALID_INSTRUCTION_VDR;
				}

				// validate the source
				const auto& src = split_line.at( 3 );

				if( is_vdr( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back(
						std::make_shared< inst_add_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
						                                      vdr::vdr_str_to_enum( src )
						)
					);

					continue;
				}

				if( is_immediate( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back( std::make_shared< inst_add_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src )
					);

					continue;
				}

				return PARSER_INVALID_INSTRUCTION_VDR;

				continue;
			}

			// syntax: sub <datatype> <destination> <source>
			// datatype := byte, word, dword, qword, float
			// destination := vdr
			// source := vdr, imm
			if( mnemonic == "sub" )
			{
				// make sure it has the correct instruction size
				if( split_line.size() != 4 )
				{
					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				// validate the datatype
				const auto& datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) )
				{
					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}


				// validate the destination
				const auto& dest = split_line.at( 2 );

				if( !is_vdr( dest ) )
				{
					return PARSER_INVALID_INSTRUCTION_VDR;
				}


				// validate the source
				const auto& src = split_line.at( 3 );

				if( is_vdr( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back(
						std::make_shared< inst_sub_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
						                                      vdr::vdr_str_to_enum( src )
						)
					);


					continue;
				}

				if( is_immediate( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back( std::make_shared< inst_sub_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src )
					);


					continue;
				}

				return PARSER_INVALID_INSTRUCTION_VDR;

				continue;
			}

			// syntax: mul <datatype> <destination> <source>
			// datatype := byte, word, dword, qword, float
			// destination := vdr
			// source := vdr, imm
			if( mnemonic == "mul" )
			{
				// make sure it has the correct instruction size
				if( split_line.size() != 4 )
				{
					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				// validate the datatype
				const auto& datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) )
				{
					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}


				// validate the destination
				const auto& dest = split_line.at( 2 );

				if( !is_vdr( dest ) )
				{
					return PARSER_INVALID_INSTRUCTION_VDR;
				}


				// validate the source
				const auto& src = split_line.at( 3 );

				if( is_vdr( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back(
						std::make_shared< inst_mul_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
						                                      vdr::vdr_str_to_enum( src )
						)
					);


					continue;
				}

				if( is_immediate( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back( std::make_shared< inst_mul_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src )
					);


					continue;
				}


				return PARSER_INVALID_INSTRUCTION_VDR;

				continue;
			}

			// syntax: mul <datatype> <destination> <source>
			// datatype := byte, word, dword, qword, float
			// destination := vdr
			// source := vdr, imm
			if( mnemonic == "div" )
			{
				// make sure it has the correct instruction size
				if( split_line.size() != 4 )
				{
					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				// validate the datatype
				const auto& datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) )
				{
					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}


				// validate the destination
				const auto& dest = split_line.at( 2 );

				if( !is_vdr( dest ) )
				{
					return PARSER_INVALID_INSTRUCTION_VDR;
				}


				// validate the source
				const auto& src = split_line.at( 3 );

				if( is_vdr( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back(
						std::make_shared< inst_div_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
						                                      vdr::vdr_str_to_enum( src )
						)
					);


					continue;
				}

				if( is_immediate( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back( std::make_shared< inst_div_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src )
					);


					continue;
				}


				return PARSER_INVALID_INSTRUCTION_VDR;

				continue;
			}

			// syntax: and <datatype> <destination> <source>
			// datatype := byte, word, dword, qword
			// destination := vdr
			// source := vdr, imm
			if( mnemonic == "and" )
			{
				// make sure it has the correct instruction size
				if( split_line.size() != 4 )
				{
					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				// validate the datatype
				const auto& datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) || datatype == "float" )
				{
					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}


				// validate the destination
				const auto& dest = split_line.at( 2 );

				if( !is_vdr( dest ) )
				{
					return PARSER_INVALID_INSTRUCTION_VDR;
				}


				// validate the source
				const auto& src = split_line.at( 3 );

				if( is_vdr( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back(
						std::make_shared< inst_and_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
						                                      vdr::vdr_str_to_enum( src )
						)
					);


					continue;
				}

				if( is_immediate( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back( std::make_shared< inst_and_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src )
					);


					continue;
				}


				return PARSER_INVALID_INSTRUCTION_VDR;

				continue;
			}

			// syntax: or <datatype> <destination> <source>
			// datatype := byte, word, dword, qword
			// destination := vdr
			// source := vdr, imm
			if( mnemonic == "or" )
			{
				// make sure it has the correct instruction size
				if( split_line.size() != 4 )
				{
					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				// validate the datatype
				const auto& datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) || datatype == "float" )
				{
					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}


				// validate the destination
				const auto& dest = split_line.at( 2 );

				if( !is_vdr( dest ) )
				{
					return PARSER_INVALID_INSTRUCTION_VDR;
				}


				// validate the source
				const auto& src = split_line.at( 3 );

				if( is_vdr( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back(
						std::make_shared< inst_or_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
						                                     vdr::vdr_str_to_enum( src )
						)
					);


					continue;
				}

				if( is_immediate( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back( std::make_shared< inst_or_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src )
					);

					continue;
				}

				return PARSER_INVALID_INSTRUCTION_VDR;

				continue;
			}

			// syntax: xor <datatype> <destination> <source>
			// datatype := byte, word, dword, qword
			// destination := vdr
			// source := vdr, imm
			if( mnemonic == "xor" )
			{
				// make sure it has the correct instruction size
				if( split_line.size() != 4 )
				{
					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				// validate the datatype
				const auto& datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) || datatype == "float" )
				{
					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}


				// validate the destination
				const auto& dest = split_line.at( 2 );

				if( !is_vdr( dest ) )
				{
					return PARSER_INVALID_INSTRUCTION_VDR;
				}

				// validate the source
				const auto& src = split_line.at( 3 );

				if( is_vdr( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back(
						std::make_shared< inst_xor_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
						                                      vdr::vdr_str_to_enum( src )
						)
					);

					continue;
				}

				if( is_immediate( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back( std::make_shared< inst_xor_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src )
					);

					continue;
				}

				return PARSER_INVALID_INSTRUCTION_VDR;

				continue;
			}

			// syntax: mov <datatype> <destination> <source>
			// datatype := byte, word, dword, qword, float
			// destination := vdr
			// source := vdr, imm
			if( mnemonic == "mov" )
			{
				// make sure it has the correct instruction size
				if( split_line.size() != 4 )
				{
					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				// validate the datatype
				const auto& datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) )
				{
					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}


				// validate the destination
				const auto& dest = split_line.at( 2 );

				if( !is_vdr( dest ) )
				{
					return PARSER_INVALID_INSTRUCTION_VDR;
				}


				// validate the source
				const auto& src = split_line.at( 3 );


				if( is_vdr( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back(
						std::make_shared< inst_mov_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
						                                      vdr::vdr_str_to_enum( src )
						)
					);

					Log::debug( "[parser::parse_file] Added inst_mov_vdr_vdr to instructions!\n" );

					continue;
				}

				if( is_immediate( src ) )
				{
					// after validating now the syntax & semantic
					// I create the instruction instance
					insts.push_back( std::make_shared< inst_mov_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src )
					);

					Log::debug( "[parser::parse_file] Added inst_mov_vdr_imm to instructions!\n" );

					continue;
				}

				Log::debug( "[parser::parse_file] Detected a invalid source vdr: %s\n", src.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;

				continue;
			}

			// syntax: println <printtype0> <msg0> ... <printtypeN> <msgN>
			// msg := string, vdr, imm
			// printtype := byte, word, dword, qword, float, hex32, hex64, ascii
			if( mnemonic == "println" )
			{
				if( split_line.size() == 1 )
				{
					Log::debug( "[parser::parse_file] Detected invalid println without messages.\n" );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				split_line.erase( split_line.begin() );

				insts.push_back( std::make_shared< inst_println >( split_line ) );

				Log::debug( "[parser::parse_file] Added inst_println to instructions!\n" );

				continue;
			}

			// syntax: jmp <rel>
			// rel := int
			if( mnemonic == "jmp" )
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[parser::parse_file] Invalid jmp syntax found: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_jmp_imm >( split_line.back() ) );

				Log::debug( "[parser::parse_file] Added inst_jmp_imm to instructions!\n" );

				continue;
			}

			// syntax: je <rel>
			// rel := int
			if( mnemonic == "je" )
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[parser::parse_file] Invalid je syntax found: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_je_imm >( split_line.back() ) );

				Log::debug( "[parser::parse_file] Added inst_je_imm to instructions!\n" );

				continue;
			}

			// syntax: jg <rel>
			// rel := int
			if( mnemonic == "jg" )
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[parser::parse_file] Invalid jg syntax found: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_jg_imm >( split_line.back() ) );

				Log::debug( "[parser::parse_file] Added inst_jg_imm to instructions!\n" );

				continue;
			}

			// syntax: jl <rel>
			// rel := int
			if( mnemonic == "jl" )
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[parser::parse_file] Invalid jl syntax found: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_jl_imm >( split_line.back() ) );

				Log::debug( "[parser::parse_file] Added inst_jl_imm to instructions!\n" );

				continue;
			}

			// syntax: jne <rel>
			// rel := int
			if( mnemonic == "jne" )
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[parser::parse_file] Invalid jne syntax found: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_jne_imm >( split_line.back() ) );

				Log::debug( "[parser::parse_file] Added inst_jne_imm to instructions!\n" );

				continue;
			}

			// syntax: jge <rel>
			// rel := int
			if( mnemonic == "jge" )
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[parser::parse_file] Invalid jge syntax found: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_jge_imm >( split_line.back() ) );

				Log::debug( "[parser::parse_file] Added inst_jge_imm to instructions!\n" );

				continue;
			}

			// syntax: jle <rel>
			// rel := int
			if( mnemonic == "jle" )
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[parser::parse_file] Invalid jle syntax found: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_jle_imm >( split_line.back() ) );

				Log::debug( "[parser::parse_file] Added inst_jle_imm to instructions!\n" );

				continue;
			}

			// syntax: sleep_ms <time>
			// time := number, vdr
			if( mnemonic == "sleep_ms" )
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[parser::parse_file] Invalid sleep syntax found: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				if( Utils::is_number( split_line.back() ) )
				{
					insts.push_back( std::make_shared< inst_sleep_ms_imm >( split_line.back() ) );

					Log::debug( "[parser::parse_file] Added inst_sleep_ms_imm to instructions!\n" );

					continue;
				}

				if( is_vdr( split_line.back() ) )
				{
					insts.push_back( std::make_shared< inst_sleep_ms_vdr >( vdr::vdr_str_to_enum( split_line.back() ) )
					);

					Log::debug( "[parser::parse_file] Added inst_sleep_ms_vdr to instructions!\n" );

					continue;
				}

				Log::debug( "[parser::parse_file] Detected invalid sleep time: %s\n", split_line.back() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			// syntax: if <datatype> <operand1> <operator> <operand2> <PREPROCESSOR-instruction-length>
			// datatype := byte, word, dword, qword, float
			// operand1 := vdr
			// operator := >, <, >=, <=, ==, !=
			// operand2 := vdr, imm
			// instruction length := int (calculated by the preprocessor)
			if( mnemonic == "if" )
			{
				// check if the size of the instruction is correct
				if( split_line.size() != 6 )
				{
					Log::debug( "[parser::parse_file] Detected invalid instruction syntax from if: %s\n", line.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto datatype = split_line.at( 1 );

				// check if a valid datatype exists
				if( !is_datatype( datatype ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid datatype in if: %s\n", datatype.c_str() );

					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}

				Log::debug( "[parser::parse_file] Extracted if-datatype: %s\n", datatype.c_str() );

				const auto operand1 = split_line.at( 2 );

				// check if operand1 is a valid vdr
				if( !is_vdr( operand1 ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid vdr in if: %s\n", operand1.c_str() );

					return PARSER_INVALID_INSTRUCTION_VDR;
				}

				Log::debug( "[parser::parse_file] Extracted if-operand1: %s\n", operand1.c_str() );

				const auto op = split_line.at( 3 );

				// check if operator is valid
				if( !is_if_operator( op ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid operator in if: %s\n", op.c_str() );

					return PARSER_INVALID_IF_OPERATOR;
				}

				Log::debug( "[parser::parse_file] Extracted if-op: %s\n", op.c_str() );

				const auto operand2 = split_line.at( 4 );

				// check if operand2 is a valid vdr or immediate
				if( is_vdr( operand2 ) || is_immediate( operand2 ) )
				{
					Log::debug( "[parser::parse_file] Extracted if-operand2: %s\n", operand2.c_str() );

					const auto if_length = std::stoi( split_line.back() );

					// add now the instruction
					insts.push_back( std::make_shared< inst_if >( datatype, operand1, op, operand2, if_length ) );

					Log::debug( "[parser::parse_file] Added inst_if to instructions.\n" );

					// go to next line
					continue;
				}


				Log::debug( "[parser::parse_file] Detected a invalid operand2 in if: %s\n", operand2.c_str() );

				return PARSER_INVALID_IF_OPERATOR;
			}

			// syntax: read_mem <datatype> <address_to_read> <result_reg>
			// datatype := byte, word, dword, qword, float
			// address_to_read := int, vdr
			// result_reg := vdr
			if( mnemonic == "read_mem" )
			{
				if( split_line.size() != 4 )
				{
					Log::debug( "[parser::parse_file] Detected invalid instruction syntax from read_mem: %s\n",
					            line.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid datatype in read_mem: %s\n", datatype.c_str() );

					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}

				Log::debug( "[parser::parse_file] Extracted datatype from read_mem: %s\n", datatype.c_str() );

				const auto result_reg = split_line.back();

				if( !is_vdr( result_reg ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid vdr in read_mem: %s\n", result_reg.c_str() );

					return PARSER_INVALID_INSTRUCTION_VDR;
				}

				const auto address_to_read = split_line.at( 2 );

				if( is_vdr( address_to_read ) )
				{
					// add now the instruction to insts
					insts.push_back( std::make_shared< inst_read_mem_vdr >( datatype, address_to_read, result_reg ) );

					Log::debug( "[parser::parse_file] Added read_mem_vdr to instructions.\n" );

					continue;
				}

				// make sure that only int and hex int are allowed, as I cannot write to a float
				if( !is_immediate( address_to_read ) || address_to_read.contains( '.' ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid immediate in read_mem: %s\n",
					            address_to_read.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SOURCE;
				}

				// add now the instruction to insts
				insts.push_back( std::make_shared< inst_read_mem_imm >( datatype, address_to_read, result_reg ) );

				Log::debug( "[parser::parse_file] Added read_mem_imm to instructions.\n" );

				continue;
			}

			// syntax: get_image_base <image_name> <result_reg>
			// datatype := str
			// result_reg := vdr
			if( mnemonic == "get_image_base" )
			{
				if( split_line.size() != 3 )
				{
					Log::debug( "[parser::parse_file] Detected invalid instruction syntax from get_image_base: %s\n",
					            line.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto image_name = split_line.at( 1 );


				Log::debug( "[parser::parse_file] Extracted image_name from get_image_base: %s\n", image_name.c_str() );

				const auto result_reg = split_line.back();

				if( !is_vdr( result_reg ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid vdr in get_image_base: %s\n", result_reg.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_VDR;
				}

				// add now the instruction to insts
				insts.push_back( std::make_shared< inst_get_image_base >( image_name, result_reg ) );

				Log::debug( "[parser::parse_file] Added get_image_base to instructions.\n" );

				continue;
			}

			// syntax: get_image_size <image_name> <result_reg>
			// datatype := str
			// result_reg := vdr
			if( mnemonic == "get_image_size" )
			{
				if( split_line.size() != 3 )
				{
					Log::debug( "[parser::parse_file] Detected invalid instruction syntax from get_image_size: %s\n",
					            line.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto image_name = split_line.at( 1 );


				Log::debug( "[parser::parse_file] Extracted image_name from get_image_size: %s\n", image_name.c_str() );

				const auto result_reg = split_line.back();

				if( !is_vdr( result_reg ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid vdr in get_image_size: %s\n", result_reg.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_VDR;
				}

				// add now the instruction to insts
				insts.push_back( std::make_shared< inst_get_image_size >( image_name, result_reg ) );

				Log::debug( "[parser::parse_file] Added get_image_size to instructions.\n" );

				continue;
			}

			// syntax: write_mem <datatype> <address_to_write> <write_value>
			// datatype := byte, word, dword, qword, float
			// address_to_write := int, vdr
			// write_value := vdr
			if( mnemonic == "write_mem" )
			{
				if( split_line.size() != 4 )
				{
					Log::debug( "[parser::parse_file] Detected invalid instruction syntax from write_mem: %s\n",
					            line.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto datatype = split_line.at( 1 );

				if( !is_datatype( datatype ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid datatype in write_mem: %s\n", datatype.c_str() );

					return PARSER_INVALID_INSTRUCTION_DATATYPE;
				}

				Log::debug( "[parser::parse_file] Extracted datatype from write_mem: %s\n", datatype.c_str() );

				const auto write_value = split_line.back();

				if( !is_vdr( write_value ) && !is_immediate( write_value ) )
				{
					Log::debug(
						"[parser::parse_file] Detected invalid write_value (not vdr nor imm) in write_mem: %s\n",
						write_value.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto address_to_write = split_line.at( 2 );

				// make sure that only int, hex and vdr's int are allowed, as I cannot write to a float
				if( ( !is_immediate( address_to_write ) || address_to_write.contains( '.' ) ) && !is_vdr(
					address_to_write
				) )
				{
					Log::debug( "[parser::parse_file] Detected invalid address_to_write in write_mem: %s\n",
					            address_to_write.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SOURCE;
				}

				// add now the instruction to insts
				insts.push_back( std::make_shared< inst_write_mem >( datatype, address_to_write, write_value ) );

				Log::debug( "[parser::parse_file] Added inst_write_mem to instructions.\n" );

				continue;
			}

			// syntax: find_signature <result_reg> <image_name> <signature>
			// result_reg := vdr
			// image_name := str
			// signature := str
			if( mnemonic == "find_signature" )
			{
				if( line.size() < 4 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto result_reg = split_line.at( 1 );

				if( !is_vdr( result_reg ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid vdr: %s\n", result_reg.c_str() );

					return PARSER_INVALID_INSTRUCTION_VDR;
				}

				Log::debug( "[parser::parse_file] Extracted vdr from find_signature: %s\n", result_reg.c_str() );

				const auto image_name = split_line.at( 2 );

				Log::debug( "[parser::parse_file] Extracted image name from find_signature: %s\n", image_name.c_str() );


				std::string sig = line.substr( line.find( image_name ) + image_name.size() + 1, line.size() );

				Log::debug( "[parser::parse_file] Extracted sig from find_signature: %s\n", sig.c_str() );


				insts.push_back(
					std::make_shared< inst_find_signature >( vdr::vdr_str_to_enum( result_reg ), image_name, sig )
				);

				Log::debug( "[parser::parse_file] Added inst_find_signature to instructions.\n" );

				continue;
			}

			// syntax: patch_bytes <address> <new_bytes>
			// address := vdr, imm
			// new_bytes := str
			if( mnemonic == "patch_bytes" )
			{
				if( line.size() < 3 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto addr = split_line.at( 1 );

				// make sure that only int, hex and vdr's int are allowed, as I cannot write to a float
				if( ( !is_immediate( addr ) || addr.contains( '.' ) ) && !is_vdr( addr ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid address in patch_bytes: %s\n", addr.c_str() );

					return PARSER_INVALID_INSTRUCTION_SOURCE;
				}

				const auto bytes = line.substr( line.find( addr ) + addr.size() + 1, line.size() );

				insts.push_back( std::make_shared< inst_patch_bytes >( addr, bytes ) );

				Log::debug( "[parser::parse_file] Added inst_patch_bytes to instructions.\n" );

				continue;
			}

			// syntax: dump <address> <size> <output_file_name>
			// address := vdr, imm
			// size := vdr, imm
			// output_file_name := str
			if( mnemonic == "dump" ) [[unlikely]]
			{
				if( split_line.size() != 4 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto address = split_line.at( 1 );

				// make sure that only int, hex and vdr's int are allowed, as I cannot read from a float
				if( ( !is_immediate( address ) || address.contains( '.' ) ) && !is_vdr( address ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid address in dump: %s\n", address.c_str() );

					return PARSER_INVALID_INSTRUCTION_SOURCE;
				}

				Log::debug( "[parser::parse_file] Extracted address: %s\n", address.c_str() );

				const auto size = split_line.at( 2 );

				// make sure that only int, hex and vdr's int are allowed, as I cannot take float as a size
				if( ( !is_immediate( size ) || size.contains( '.' ) ) && !is_vdr( size ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid size in dump: %s\n", size.c_str() );

					return PARSER_INVALID_INSTRUCTION_SOURCE;
				}

				Log::debug( "[parser::parse_file] Extracted size: %s\n", size.c_str() );

				const auto output_file_name = split_line.back();

				Log::debug( "[parser::parse_file] Extracted output file name: %s\n", output_file_name.c_str() );

				insts.push_back( std::make_shared< inst_dump >( address, size, output_file_name ) );

				Log::debug( "[parser::parse_file] Added inst_dump to instructions.\n" );

				continue;
			}

			// syntax: suspend_process
			if( mnemonic == "suspend_process" ) [[unlikely]]
			{
				if( split_line.size() != 1 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_suspend_process >() );

				Log::debug( "[parser::parse_file] Added suspend_process to instructions.\n" );

				continue;
			}

			// syntax: resume_process
			if( mnemonic == "resume_process" ) [[unlikely]]
			{
				if( split_line.size() != 1 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_resume_process >() );

				Log::debug( "[parser::parse_file] Added inst_resume_process to instructions.\n" );

				continue;
			}

			// syntax: inject_dll <dll_path> <injection_method>
			// dll_path := str
			// injection_method := str
			if( mnemonic == "inject_dll" ) [[unlikely ]]
			{
				if( split_line.size() != 3 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto dll_path = split_line.at( 1 );

				Log::debug( "[parser::parse_file] Extracted dll path for injection: %s\n", dll_path.c_str() );

				const auto injection_method = split_line.back();

				Log::debug( "[parser::parse_file] Extracted injection method for injection: %s\n", injection_method );

				insts.push_back( std::make_shared< inst_inject_dll >( dll_path, injection_method ) );

				Log::debug( "[parser::parse_file] Added inst_inject_dll to instructions.\n" );

				continue;
			}

			// syntax: terminate_process <pid> 
			// pid := vdr, imm
			if( mnemonic == "terminate_process" ) [[unlikely]]
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto pid = split_line.back();

				Log::debug( "[parser::parse_file] Extracted pid: %s\n", pid.c_str() );

				// make sure that only int, hex and vdr's int are allowed, as I cannot take float as a pid
				if( ( !is_immediate( pid ) || pid.contains( '.' ) ) && !is_vdr( pid ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid pid in terminate_process: %s\n", pid.c_str() );

					return PARSER_INVALID_INSTRUCTION_SOURCE;
				}

				insts.push_back( std::make_shared< inst_terminate_process >( pid ) );

				Log::debug( "[parser::parse_file] Added inst_terminate_process to instructions.\n" );

				continue;
			}

			// syntax: show_threads_info
			if( mnemonic == "show_threads_info" ) [[unlikely]]
			{
				if( split_line.size() != 1 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_show_threads_info >() );

				Log::debug( "[parser::parse_file] Added inst_show_threads_info to instructions.\n" );

				continue;
			}

			// syntax: show_images_info
			if( mnemonic == "show_images_info" ) [[unlikely]]
			{
				if( split_line.size() != 1 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_show_images_info >() );

				Log::debug( "[parser::parse_file] Added inst_show_images_info to instructions.\n" );

				continue;
			}

			// syntax: cmp <operand1> <operand2>
			// operand1 := vdr, imm
			// operand2 := vdr, imm
			if( mnemonic == "cmp" )
			{
				if( split_line.size() != 3 )
				{
					Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto operand1 = split_line.at( 1 );

				if( !is_vdr( operand1 ) && !is_immediate( operand1 ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid operand1, not a vdr or imm: %s\n",
					            operand1.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				const auto operand2 = split_line.back();

				if( !is_vdr( operand2 ) && !is_immediate( operand2 ) )
				{
					Log::debug( "[parser::parse_file] Detected invalid operand2, not a vdr or imm: %s\n",
					            operand1.c_str()
					);

					return PARSER_INVALID_INSTRUCTION_SYNTAX;
				}

				insts.push_back( std::make_shared< inst_cmp >( operand1, operand2 ) );

				Log::debug( "[parser::parse_file] Added inst_cmp to instructions.\n" );

				continue;
			}

			// Unknown instruction found in file
			Log::debug( "[parser::parse_file] Unknown instruction found: %s\n", line.c_str() );

			return PARSER_UNKNOWN_INSTRUCTION_FOUND;
		}
		
		while( current_id != id )
			Sleep( 1 );

		mtx.lock();

		th_insts.reserve( th_insts.size() + insts.size() );

		th_insts.insert( th_insts.end(), insts.begin(), insts.end() );

		mtx.unlock();

		current_id++;

		return PARSER_SUCCESS;
	};

	std::vector< std::thread > threads;

	for( size_t idx = 0; idx < thread_count; idx++ )
		if( idx + 1 == thread_count )
			threads.emplace_back( parse_code, chunk_size * idx, chunk_size * idx + chunk_size + chunk_leftover, idx );
		else
			threads.emplace_back( parse_code, chunk_size * idx, chunk_size * idx + chunk_size, idx );

	for( auto& th : threads )
		th.join();

	this->m_instructions = th_insts;

	return PARSER_SUCCESS;
}

EStatusCode parser::parse_file_multithreaded_chunking( const std::string& path_to_file )
{
	return this->parse_file_multithreaded_chunking( path_to_file, std::thread::hardware_concurrency() );
}

EStatusCode parser::parse_file( const std::string& path_to_file )
{
	// Check if the path exists
	if( !Utils::does_file_exists( path_to_file ) )
		return PARSER_FILE_PATH_DOES_NOT_EXIST;

	Log::info( "[parser::parse_file] Parsing now %s\n", path_to_file.c_str() );

	// Read content from code file
	auto code = Utils::read_file( path_to_file );

	// If code file is empty, no need to process it
	if( code.empty() )
		return PARSER_PRP_FILE_IS_EMPTY;

	// Vector which holds all parsed instructions from the preprocessed file (.prp)
	std::vector< std::shared_ptr< instruction > > insts;

	// Check syntax and semantics
	for( auto& line : code )
	{
		// skip empty lines
		if( line.empty() )
			continue;

		// make line lowercase
		Utils::string_to_lower( line );

		Log::debug( "[parser::parse_file] Current parsed line: %s\n", line.c_str() );

		// split line into multiple strings
		auto split_line = Utils::split_string( line );

		// extract mnemonic from line
		const auto& mnemonic = split_line.front();

		Log::debug( "[parser::parse_file] Extracted mnemonic: %s\n", mnemonic.c_str() );

		// syntax: rpcae
		if( mnemonic == "rpcae" )
		{
			insts.push_back( std::make_shared< inst_rpcae >() );

			Log::debug( "[parser::parse_file] Added inst_rpcae to instructions!\n" );

			continue;
		}

		// syntax: inc <destination>
		// destination := vdr
		if( mnemonic == "inc" )
		{
			if( const auto ret = is_valid_two_part_instruction( line ); ret == PARSER_SUCCESS )
			{
				insts.push_back( std::make_shared< inst_inc_vdr >( vdr::vdr_str_to_enum( split_line.back() ) ) );

				Log::debug( "[parser::parse_file] Added inst_inc_vdr to instructions!\n" );
			}
			else
				return ret;

			continue;
		}

		// syntax: dec <destination>
		// destination := vdr
		if( mnemonic == "dec" )
		{
			if( const auto ret = is_valid_two_part_instruction( line ); ret == PARSER_SUCCESS )
			{
				insts.push_back( std::make_shared< inst_dec_vdr >( vdr::vdr_str_to_enum( split_line.back() ) ) );

				Log::debug( "[parser::parse_file] Added inst_dec_vdr to instructions!\n" );
			}
			else
				return ret;

			continue;
		}

		// syntax: add <datatype> <destination> <source>
		// datatype := byte, word, dword, qword, float
		// destination := vdr
		// source := vdr, imm
		if( mnemonic == "add" )
		{
			// make sure it has the correct instruction size
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction size: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			// validate the datatype
			const auto& datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid datatype: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype: %s\n", datatype.c_str() );

			// validate the destination
			const auto& dest = split_line.at( 2 );

			if( !is_vdr( dest ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid destination vdr: %s\n", dest.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted destination: %s\n", dest.c_str() );

			// validate the source
			const auto& src = split_line.at( 3 );

			Log::debug( "[parser::parse_file] Extracted source: %s\n", src.c_str() );

			if( is_vdr( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back(
					std::make_shared< inst_add_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
					                                      vdr::vdr_str_to_enum( src )
					)
				);

				Log::debug( "[parser::parse_file] Added inst_add_vdr_vdr to instructions!\n" );

				continue;
			}

			if( is_immediate( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back( std::make_shared< inst_add_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src ) );

				Log::debug( "[parser::parse_file] Added inst_add_vdr_imm to instructions!\n" );

				continue;
			}

			Log::debug( "[parser::parse_file] Detected a invalid source vdr: %s\n", src.c_str() );

			return PARSER_INVALID_INSTRUCTION_VDR;
		}

		// syntax: sub <datatype> <destination> <source>
		// datatype := byte, word, dword, qword, float
		// destination := vdr
		// source := vdr, imm
		if( mnemonic == "sub" )
		{
			// make sure it has the correct instruction size
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction size: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			// validate the datatype
			const auto& datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid datatype: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype: %s\n", datatype.c_str() );

			// validate the destination
			const auto& dest = split_line.at( 2 );

			if( !is_vdr( dest ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid destination vdr: %s\n", dest.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted destination: %s\n", dest.c_str() );

			// validate the source
			const auto& src = split_line.at( 3 );

			if( is_vdr( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back(
					std::make_shared< inst_sub_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
					                                      vdr::vdr_str_to_enum( src )
					)
				);

				Log::debug( "[parser::parse_file] Added inst_sub_vdr_vdr to instructions!\n" );

				continue;
			}

			if( is_immediate( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back( std::make_shared< inst_sub_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src ) );

				Log::debug( "[parser::parse_file] Added inst_sub_vdr_imm to instructions!\n" );

				continue;
			}

			Log::debug( "[parser::parse_file] Detected a invalid source vdr: %s\n", src.c_str() );

			return PARSER_INVALID_INSTRUCTION_VDR;

			continue;
		}

		// syntax: mul <datatype> <destination> <source>
		// datatype := byte, word, dword, qword, float
		// destination := vdr
		// source := vdr, imm
		if( mnemonic == "mul" )
		{
			// make sure it has the correct instruction size
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction size: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			// validate the datatype
			const auto& datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid datatype: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype: %s\n", datatype.c_str() );

			// validate the destination
			const auto& dest = split_line.at( 2 );

			if( !is_vdr( dest ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid destination vdr: %s\n", dest.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted destination: %s\n", dest.c_str() );

			// validate the source
			const auto& src = split_line.at( 3 );

			if( is_vdr( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back(
					std::make_shared< inst_mul_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
					                                      vdr::vdr_str_to_enum( src )
					)
				);

				Log::debug( "[parser::parse_file] Added inst_mul_vdr_vdr to instructions!\n" );

				continue;
			}

			if( is_immediate( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back( std::make_shared< inst_mul_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src ) );

				Log::debug( "[parser::parse_file] Added inst_mul_vdr_imm to instructions!\n" );

				continue;
			}

			Log::debug( "[parser::parse_file] Detected a invalid source vdr: %s\n", src.c_str() );

			return PARSER_INVALID_INSTRUCTION_VDR;

			continue;
		}

		// syntax: mul <datatype> <destination> <source>
		// datatype := byte, word, dword, qword, float
		// destination := vdr
		// source := vdr, imm
		if( mnemonic == "div" )
		{
			// make sure it has the correct instruction size
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction size: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			// validate the datatype
			const auto& datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid datatype: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype: %s\n", datatype.c_str() );

			// validate the destination
			const auto& dest = split_line.at( 2 );

			if( !is_vdr( dest ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid destination vdr: %s\n", dest.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted destination: %s\n", dest.c_str() );

			// validate the source
			const auto& src = split_line.at( 3 );

			if( is_vdr( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back(
					std::make_shared< inst_div_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
					                                      vdr::vdr_str_to_enum( src )
					)
				);

				Log::debug( "[parser::parse_file] Added inst_div_vdr_vdr to instructions!\n" );

				continue;
			}

			if( is_immediate( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back( std::make_shared< inst_div_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src ) );

				Log::debug( "[parser::parse_file] Added inst_div_vdr_imm to instructions!\n" );

				continue;
			}

			Log::debug( "[parser::parse_file] Detected a invalid source vdr: %s\n", src.c_str() );

			return PARSER_INVALID_INSTRUCTION_VDR;

			continue;
		}

		// syntax: and <datatype> <destination> <source>
		// datatype := byte, word, dword, qword
		// destination := vdr
		// source := vdr, imm
		if( mnemonic == "and" )
		{
			// make sure it has the correct instruction size
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction size: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			// validate the datatype
			const auto& datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) || datatype == "float" )
			{
				Log::debug( "[parser::parse_file] Detected a invalid datatype: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype: %s\n", datatype.c_str() );

			// validate the destination
			const auto& dest = split_line.at( 2 );

			if( !is_vdr( dest ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid destination vdr: %s\n", dest.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted destination: %s\n", dest.c_str() );

			// validate the source
			const auto& src = split_line.at( 3 );

			if( is_vdr( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back(
					std::make_shared< inst_and_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
					                                      vdr::vdr_str_to_enum( src )
					)
				);

				Log::debug( "[parser::parse_file] Added inst_and_vdr_vdr to instructions!\n" );

				continue;
			}

			if( is_immediate( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back( std::make_shared< inst_and_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src ) );

				Log::debug( "[parser::parse_file] Added inst_and_vdr_imm to instructions!\n" );

				continue;
			}

			Log::debug( "[parser::parse_file] Detected a invalid source vdr: %s\n", src.c_str() );

			return PARSER_INVALID_INSTRUCTION_VDR;

			continue;
		}

		// syntax: or <datatype> <destination> <source>
		// datatype := byte, word, dword, qword
		// destination := vdr
		// source := vdr, imm
		if( mnemonic == "or" )
		{
			// make sure it has the correct instruction size
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction size: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			// validate the datatype
			const auto& datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) || datatype == "float" )
			{
				Log::debug( "[parser::parse_file] Detected a invalid datatype: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype: %s\n", datatype.c_str() );

			// validate the destination
			const auto& dest = split_line.at( 2 );

			if( !is_vdr( dest ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid destination vdr: %s\n", dest.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted destination: %s\n", dest.c_str() );

			// validate the source
			const auto& src = split_line.at( 3 );

			if( is_vdr( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back(
					std::make_shared< inst_or_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
					                                     vdr::vdr_str_to_enum( src )
					)
				);

				Log::debug( "[parser::parse_file] Added inst_or_vdr_vdr to instructions!\n" );

				continue;
			}

			if( is_immediate( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back( std::make_shared< inst_or_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src ) );

				Log::debug( "[parser::parse_file] Added inst_or_vdr_imm to instructions!\n" );

				continue;
			}

			Log::debug( "[parser::parse_file] Detected a invalid source vdr: %s\n", src.c_str() );

			return PARSER_INVALID_INSTRUCTION_VDR;

			continue;
		}

		// syntax: xor <datatype> <destination> <source>
		// datatype := byte, word, dword, qword
		// destination := vdr
		// source := vdr, imm
		if( mnemonic == "xor" )
		{
			// make sure it has the correct instruction size
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction size: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			// validate the datatype
			const auto& datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) || datatype == "float" )
			{
				Log::debug( "[parser::parse_file] Detected a invalid datatype: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype: %s\n", datatype.c_str() );

			// validate the destination
			const auto& dest = split_line.at( 2 );

			if( !is_vdr( dest ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid destination vdr: %s\n", dest.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted destination: %s\n", dest.c_str() );

			// validate the source
			const auto& src = split_line.at( 3 );

			if( is_vdr( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back(
					std::make_shared< inst_xor_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
					                                      vdr::vdr_str_to_enum( src )
					)
				);

				Log::debug( "[parser::parse_file] Added inst_xor_vdr_vdr to instructions!\n" );

				continue;
			}

			if( is_immediate( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back( std::make_shared< inst_xor_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src ) );

				Log::debug( "[parser::parse_file] Added inst_xor_vdr_imm to instructions!\n" );

				continue;
			}

			Log::debug( "[parser::parse_file] Detected a invalid source vdr: %s\n", src.c_str() );

			return PARSER_INVALID_INSTRUCTION_VDR;

			continue;
		}

		// syntax: mov <datatype> <destination> <source>
		// datatype := byte, word, dword, qword, float
		// destination := vdr
		// source := vdr, imm
		if( mnemonic == "mov" )
		{
			// make sure it has the correct instruction size
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction size: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			// validate the datatype
			const auto& datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid datatype: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype: %s\n", datatype.c_str() );

			// validate the destination
			const auto& dest = split_line.at( 2 );

			if( !is_vdr( dest ) )
			{
				Log::debug( "[parser::parse_file] Detected a invalid destination vdr: %s\n", dest.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted destination: %s\n", dest.c_str() );

			// validate the source
			const auto& src = split_line.at( 3 );

			Log::debug( "[parser::parse_file] Extracted source: %s\n", src.c_str() );

			if( is_vdr( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back(
					std::make_shared< inst_mov_vdr_vdr >( datatype, vdr::vdr_str_to_enum( dest ),
					                                      vdr::vdr_str_to_enum( src )
					)
				);

				Log::debug( "[parser::parse_file] Added inst_mov_vdr_vdr to instructions!\n" );

				continue;
			}

			if( is_immediate( src ) )
			{
				// after validating now the syntax & semantic
				// I create the instruction instance
				insts.push_back( std::make_shared< inst_mov_vdr_imm >( datatype, vdr::vdr_str_to_enum( dest ), src ) );

				Log::debug( "[parser::parse_file] Added inst_mov_vdr_imm to instructions!\n" );

				continue;
			}

			Log::debug( "[parser::parse_file] Detected a invalid source vdr: %s\n", src.c_str() );

			return PARSER_INVALID_INSTRUCTION_VDR;

			continue;
		}

		// syntax: println <printtype0> <msg0> ... <printtypeN> <msgN>
		// msg := string, vdr, imm
		// printtype := byte, word, dword, qword, float, hex32, hex64, ascii
		if( mnemonic == "println" )
		{
			if( split_line.size() == 1 )
			{
				Log::debug( "[parser::parse_file] Detected invalid println without messages.\n" );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			split_line.erase( split_line.begin() );

			insts.push_back( std::make_shared< inst_println >( split_line ) );

			Log::debug( "[parser::parse_file] Added inst_println to instructions!\n" );

			continue;
		}

		// syntax: jmp <rel>
		// rel := int
		if( mnemonic == "jmp" )
		{
			if( split_line.size() != 2 )
			{
				Log::debug( "[parser::parse_file] Invalid jmp syntax found: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_jmp_imm >( split_line.back() ) );

			Log::debug( "[parser::parse_file] Added inst_jmp_imm to instructions!\n" );

			continue;
		}

		// syntax: je <rel>
		// rel := int
		if( mnemonic == "je" )
		{
			if( split_line.size() != 2 )
			{
				Log::debug( "[parser::parse_file] Invalid je syntax found: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_je_imm >( split_line.back() ) );

			Log::debug( "[parser::parse_file] Added inst_je_imm to instructions!\n" );

			continue;
		}

		// syntax: jg <rel>
		// rel := int
		if( mnemonic == "jg" )
		{
			if( split_line.size() != 2 )
			{
				Log::debug( "[parser::parse_file] Invalid jg syntax found: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_jg_imm >( split_line.back() ) );

			Log::debug( "[parser::parse_file] Added inst_jg_imm to instructions!\n" );

			continue;
		}

		// syntax: jl <rel>
		// rel := int
		if( mnemonic == "jl" )
		{
			if( split_line.size() != 2 )
			{
				Log::debug( "[parser::parse_file] Invalid jl syntax found: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_jl_imm >( split_line.back() ) );

			Log::debug( "[parser::parse_file] Added inst_jl_imm to instructions!\n" );

			continue;
		}

		// syntax: jne <rel>
		// rel := int
		if( mnemonic == "jne" )
		{
			if( split_line.size() != 2 )
			{
				Log::debug( "[parser::parse_file] Invalid jne syntax found: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_jne_imm >( split_line.back() ) );

			Log::debug( "[parser::parse_file] Added inst_jne_imm to instructions!\n" );

			continue;
		}

		// syntax: jge <rel>
		// rel := int
		if( mnemonic == "jge" )
		{
			if( split_line.size() != 2 )
			{
				Log::debug( "[parser::parse_file] Invalid jge syntax found: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_jge_imm >( split_line.back() ) );

			Log::debug( "[parser::parse_file] Added inst_jge_imm to instructions!\n" );

			continue;
		}

		// syntax: jle <rel>
		// rel := int
		if( mnemonic == "jle" )
		{
			if( split_line.size() != 2 )
			{
				Log::debug( "[parser::parse_file] Invalid jle syntax found: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_jle_imm >( split_line.back() ) );

			Log::debug( "[parser::parse_file] Added inst_jle_imm to instructions!\n" );

			continue;
		}

		// syntax: sleep_ms <time>
		// time := number, vdr
		if( mnemonic == "sleep_ms" )
		{
			if( split_line.size() != 2 )
			{
				Log::debug( "[parser::parse_file] Invalid sleep syntax found: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			if( Utils::is_number( split_line.back() ) )
			{
				insts.push_back( std::make_shared< inst_sleep_ms_imm >( split_line.back() ) );

				Log::debug( "[parser::parse_file] Added inst_sleep_ms_imm to instructions!\n" );

				continue;
			}

			if( is_vdr( split_line.back() ) )
			{
				insts.push_back( std::make_shared< inst_sleep_ms_vdr >( vdr::vdr_str_to_enum( split_line.back() ) ) );

				Log::debug( "[parser::parse_file] Added inst_sleep_ms_vdr to instructions!\n" );

				continue;
			}

			Log::debug( "[parser::parse_file] Detected invalid sleep time: %s\n", split_line.back() );

			return PARSER_INVALID_INSTRUCTION_SYNTAX;
		}

		// syntax: if <datatype> <operand1> <operator> <operand2> <PREPROCESSOR-instruction-length>
		// datatype := byte, word, dword, qword, float
		// operand1 := vdr
		// operator := >, <, >=, <=, ==, !=
		// operand2 := vdr, imm
		// instruction length := int (calculated by the preprocessor)
		if( mnemonic == "if" )
		{
			// check if the size of the instruction is correct
			if( split_line.size() != 6 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction syntax from if: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto datatype = split_line.at( 1 );

			// check if a valid datatype exists
			if( !is_datatype( datatype ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid datatype in if: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted if-datatype: %s\n", datatype.c_str() );

			const auto operand1 = split_line.at( 2 );

			// check if operand1 is a valid vdr
			if( !is_vdr( operand1 ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid vdr in if: %s\n", operand1.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted if-operand1: %s\n", operand1.c_str() );

			const auto op = split_line.at( 3 );

			// check if operator is valid
			if( !is_if_operator( op ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid operator in if: %s\n", op.c_str() );

				return PARSER_INVALID_IF_OPERATOR;
			}

			Log::debug( "[parser::parse_file] Extracted if-op: %s\n", op.c_str() );

			const auto operand2 = split_line.at( 4 );

			// check if operand2 is a valid vdr or immediate
			if( is_vdr( operand2 ) || is_immediate( operand2 ) )
			{
				Log::debug( "[parser::parse_file] Extracted if-operand2: %s\n", operand2.c_str() );

				const auto if_length = std::stoi( split_line.back() );

				// add now the instruction
				insts.push_back( std::make_shared< inst_if >( datatype, operand1, op, operand2, if_length ) );

				Log::debug( "[parser::parse_file] Added inst_if to instructions.\n" );

				// go to next line
				continue;
			}


			Log::debug( "[parser::parse_file] Detected a invalid operand2 in if: %s\n", operand2.c_str() );

			return PARSER_INVALID_IF_OPERATOR;
		}

		// syntax: read_mem <datatype> <address_to_read> <result_reg>
		// datatype := byte, word, dword, qword, float
		// address_to_read := int, vdr
		// result_reg := vdr
		if( mnemonic == "read_mem" )
		{
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction syntax from read_mem: %s\n", line.c_str()
				);

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid datatype in read_mem: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype from read_mem: %s\n", datatype.c_str() );

			const auto result_reg = split_line.back();

			if( !is_vdr( result_reg ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid vdr in read_mem: %s\n", result_reg.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			const auto address_to_read = split_line.at( 2 );

			if( is_vdr( address_to_read ) )
			{
				// add now the instruction to insts
				insts.push_back( std::make_shared< inst_read_mem_vdr >( datatype, address_to_read, result_reg ) );

				Log::debug( "[parser::parse_file] Added read_mem_vdr to instructions.\n" );

				continue;
			}

			// make sure that only int and hex int are allowed, as I cannot write to a float
			if( !is_immediate( address_to_read ) || address_to_read.contains( '.' ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid immediate in read_mem: %s\n", address_to_read.c_str()
				);

				return PARSER_INVALID_INSTRUCTION_SOURCE;
			}

			// add now the instruction to insts
			insts.push_back( std::make_shared< inst_read_mem_imm >( datatype, address_to_read, result_reg ) );

			Log::debug( "[parser::parse_file] Added read_mem_imm to instructions.\n" );

			continue;
		}

		// syntax: get_image_base <image_name> <result_reg>
		// datatype := str
		// result_reg := vdr
		if( mnemonic == "get_image_base" )
		{
			if( split_line.size() != 3 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction syntax from get_image_base: %s\n",
				            line.c_str()
				);

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto image_name = split_line.at( 1 );


			Log::debug( "[parser::parse_file] Extracted image_name from get_image_base: %s\n", image_name.c_str() );

			const auto result_reg = split_line.back();

			if( !is_vdr( result_reg ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid vdr in get_image_base: %s\n", result_reg.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			// add now the instruction to insts
			insts.push_back( std::make_shared< inst_get_image_base >( image_name, result_reg ) );

			Log::debug( "[parser::parse_file] Added get_image_base to instructions.\n" );

			continue;
		}

		// syntax: get_image_size <image_name> <result_reg>
		// datatype := str
		// result_reg := vdr
		if( mnemonic == "get_image_size" )
		{
			if( split_line.size() != 3 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction syntax from get_image_size: %s\n",
				            line.c_str()
				);

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto image_name = split_line.at( 1 );


			Log::debug( "[parser::parse_file] Extracted image_name from get_image_size: %s\n", image_name.c_str() );

			const auto result_reg = split_line.back();

			if( !is_vdr( result_reg ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid vdr in get_image_size: %s\n", result_reg.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			// add now the instruction to insts
			insts.push_back( std::make_shared< inst_get_image_size >( image_name, result_reg ) );

			Log::debug( "[parser::parse_file] Added get_image_size to instructions.\n" );

			continue;
		}

		// syntax: write_mem <datatype> <address_to_write> <write_value>
		// datatype := byte, word, dword, qword, float
		// address_to_write := int, vdr
		// write_value := vdr
		if( mnemonic == "write_mem" )
		{
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Detected invalid instruction syntax from write_mem: %s\n",
				            line.c_str()
				);

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto datatype = split_line.at( 1 );

			if( !is_datatype( datatype ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid datatype in write_mem: %s\n", datatype.c_str() );

				return PARSER_INVALID_INSTRUCTION_DATATYPE;
			}

			Log::debug( "[parser::parse_file] Extracted datatype from write_mem: %s\n", datatype.c_str() );

			const auto write_value = split_line.back();

			if( !is_vdr( write_value ) && !is_immediate( write_value ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid write_value (not vdr nor imm) in write_mem: %s\n",
				            write_value.c_str()
				);

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto address_to_write = split_line.at( 2 );

			// make sure that only int, hex and vdr's int are allowed, as I cannot write to a float
			if( ( !is_immediate( address_to_write ) || address_to_write.contains( '.' ) ) && ! is_vdr( address_to_write
			) )
			{
				Log::debug( "[parser::parse_file] Detected invalid address_to_write in write_mem: %s\n",
				            address_to_write.c_str()
				);

				return PARSER_INVALID_INSTRUCTION_SOURCE;
			}

			// add now the instruction to insts
			insts.push_back( std::make_shared< inst_write_mem >( datatype, address_to_write, write_value ) );

			Log::debug( "[parser::parse_file] Added inst_write_mem to instructions.\n" );

			continue;
		}

		// syntax: find_signature <result_reg> <image_name> <signature>
		// result_reg := vdr
		// image_name := str
		// signature := str
		if( mnemonic == "find_signature" )
		{
			if( line.size() < 4 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto result_reg = split_line.at( 1 );

			if( !is_vdr( result_reg ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid vdr: %s\n", result_reg.c_str() );

				return PARSER_INVALID_INSTRUCTION_VDR;
			}

			Log::debug( "[parser::parse_file] Extracted vdr from find_signature: %s\n", result_reg.c_str() );

			const auto image_name = split_line.at( 2 );

			Log::debug( "[parser::parse_file] Extracted image name from find_signature: %s\n", image_name.c_str() );


			std::string sig = line.substr( line.find( image_name ) + image_name.size() + 1, line.size() );

			Log::debug( "[parser::parse_file] Extracted sig from find_signature: %s\n", sig.c_str() );


			insts.push_back(
				std::make_shared< inst_find_signature >( vdr::vdr_str_to_enum( result_reg ), image_name, sig )
			);

			Log::debug( "[parser::parse_file] Added inst_find_signature to instructions.\n" );

			continue;
		}

		// syntax: patch_bytes <address> <new_bytes>
		// address := vdr, imm
		// new_bytes := str
		if( mnemonic == "patch_bytes" )
		{
			if( line.size() < 3 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto addr = split_line.at( 1 );

			// make sure that only int, hex and vdr's int are allowed, as I cannot write to a float
			if( ( !is_immediate( addr ) || addr.contains( '.' ) ) && !is_vdr( addr ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid address in patch_bytes: %s\n", addr.c_str() );

				return PARSER_INVALID_INSTRUCTION_SOURCE;
			}

			const auto bytes = line.substr( line.find( addr ) + addr.size() + 1, line.size() );

			insts.push_back( std::make_shared< inst_patch_bytes >( addr, bytes ) );

			Log::debug( "[parser::parse_file] Added inst_patch_bytes to instructions.\n" );

			continue;
		}

		// syntax: dump <address> <size> <output_file_name>
		// address := vdr, imm
		// size := vdr, imm
		// output_file_name := str
		if( mnemonic == "dump" )
		{
			if( split_line.size() != 4 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto address = split_line.at( 1 );

			// make sure that only int, hex and vdr's int are allowed, as I cannot read from a float
			if( ( !is_immediate( address ) || address.contains( '.' ) ) && !is_vdr( address ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid address in dump: %s\n", address.c_str() );

				return PARSER_INVALID_INSTRUCTION_SOURCE;
			}

			Log::debug( "[parser::parse_file] Extracted address: %s\n", address.c_str() );

			const auto size = split_line.at( 2 );

			// make sure that only int, hex and vdr's int are allowed, as I cannot take float as a size
			if( ( !is_immediate( size ) || size.contains( '.' ) ) && !is_vdr( size ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid size in dump: %s\n", size.c_str() );

				return PARSER_INVALID_INSTRUCTION_SOURCE;
			}

			Log::debug( "[parser::parse_file] Extracted size: %s\n", size.c_str() );

			const auto output_file_name = split_line.back();

			Log::debug( "[parser::parse_file] Extracted output file name: %s\n", output_file_name.c_str() );

			insts.push_back( std::make_shared< inst_dump >( address, size, output_file_name ) );

			Log::debug( "[parser::parse_file] Added inst_dump to instructions.\n" );

			continue;
		}

		// syntax: suspend_process
		if( mnemonic == "suspend_process" )
		{
			if( split_line.size() != 1 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_suspend_process >() );

			Log::debug( "[parser::parse_file] Added suspend_process to instructions.\n" );

			continue;
		}

		// syntax: resume_process
		if( mnemonic == "resume_process" )
		{
			if( split_line.size() != 1 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_resume_process >() );

			Log::debug( "[parser::parse_file] Added inst_resume_process to instructions.\n" );

			continue;
		}

		// syntax: inject_dll <dll_path> <injection_method>
		// dll_path := str
		// injection_method := str
		if( mnemonic == "inject_dll" )
		{
			if( split_line.size() != 3 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto dll_path = split_line.at( 1 );

			Log::debug( "[parser::parse_file] Extracted dll path for injection: %s\n", dll_path.c_str() );

			const auto injection_method = split_line.back();

			Log::debug( "[parser::parse_file] Extracted injection method for injection: %s\n", injection_method );

			insts.push_back( std::make_shared< inst_inject_dll >( dll_path, injection_method ) );

			Log::debug( "[parser::parse_file] Added inst_inject_dll to instructions.\n" );

			continue;
		}

		// syntax: terminate_process <pid> 
		// pid := vdr, imm
		if( mnemonic == "terminate_process" )
		{
			if( split_line.size() != 2 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto pid = split_line.back();

			Log::debug( "[parser::parse_file] Extracted pid: %s\n", pid.c_str() );

			// make sure that only int, hex and vdr's int are allowed, as I cannot take float as a pid
			if( ( !is_immediate( pid ) || pid.contains( '.' ) ) && !is_vdr( pid ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid pid in terminate_process: %s\n", pid.c_str() );

				return PARSER_INVALID_INSTRUCTION_SOURCE;
			}

			insts.push_back( std::make_shared< inst_terminate_process >( pid ) );

			Log::debug( "[parser::parse_file] Added inst_terminate_process to instructions.\n" );

			continue;
		}

		// syntax: show_threads_info
		if( mnemonic == "show_threads_info" )
		{
			if( split_line.size() != 1 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_show_threads_info >() );

			Log::debug( "[parser::parse_file] Added inst_show_threads_info to instructions.\n" );

			continue;
		}

		// syntax: show_images_info
		if( mnemonic == "show_images_info" )
		{
			if( split_line.size() != 1 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_show_images_info >() );

			Log::debug( "[parser::parse_file] Added inst_show_images_info to instructions.\n" );

			continue;
		}

		// syntax: cmp <operand1> <operand2>
		// operand1 := vdr, imm
		// operand2 := vdr, imm
		if( mnemonic == "cmp" )
		{
			if( split_line.size() != 3 )
			{
				Log::debug( "[parser::parse_file] Invalid instruction syntax: %s\n", line.c_str() );

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto operand1 = split_line.at( 1 );

			if( !is_vdr( operand1 ) && !is_immediate( operand1 ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid operand1, not a vdr or imm: %s\n", operand1.c_str()
				);

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			const auto operand2 = split_line.back();

			if( !is_vdr( operand2 ) && !is_immediate( operand2 ) )
			{
				Log::debug( "[parser::parse_file] Detected invalid operand2, not a vdr or imm: %s\n", operand1.c_str()
				);

				return PARSER_INVALID_INSTRUCTION_SYNTAX;
			}

			insts.push_back( std::make_shared< inst_cmp >( operand1, operand2 ) );

			Log::debug( "[parser::parse_file] Added inst_cmp to instructions.\n" );

			continue;
		}

		// Unknown instruction found in file
		Log::debug( "[parser::parse_file] Unknown instruction found: %s\n", line.c_str() );

		return PARSER_UNKNOWN_INSTRUCTION_FOUND;
	}

	// after having all instructions validated and semantically analyzed
	// I set the instructions to the class member of the parser
	// But before setting I want to make sure to clear() the old vector 
	// This automatically free's old allocated pointer from old instructions

	if( !this->m_instructions.empty() )
		this->m_instructions.clear();

	this->m_instructions = insts;
	
	return PARSER_SUCCESS;
}
