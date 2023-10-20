#include "preprocessor.hpp"

#include <filesystem>
#include <regex>
#include <vector>
#include <algorithm>
#include <forward_list>
#include <execution>
#include <random>

#include "utils.hpp"

#include "logging.hpp"

std::string preprocessor::make_macro_parameter_marker( const size_t parameter_index ) const
{
	return std::vformat( "{}{:d}", std::make_format_args( PREPROCESSOR_MACRO_PARAM_STATIC, parameter_index ) );
}


EStatusCode preprocessor::process_file( const std::string& path_to_file )
{
	// Check if the path exists
	if( !Utils::does_file_exists( path_to_file ) )
		return PREPROCESS_CODE_FILE_PATH_DOES_NOT_EXIST;

	Log::info( "[preprocessor::process_file] Preprocessing now %s\n", path_to_file.c_str() );

	// Read content from code file
	auto code = Utils::read_file( path_to_file );

	// If code file is empty, no need to process it
	if( code.empty() )
		return PREPROCESS_CODE_FILE_IS_EMPTY;

	// Vector which holds all original and their replacements as pairs
	std::vector< std::pair< std::string, std::string > > inst_alias;

	// Vector which holds all macros by name as a string and their instructions as a vector
	std::vector< std::pair< std::string, std::vector< std::string > > > macros;

	// bool which indicates if a macro is currently processed
	// needed for extraction of the instructions of the macro
	bool is_macro = false;

	// string which holds the current macro name
	std::string current_macro_name = {};

	// Vector which holds the current macro instructions
	std::vector< std::string > current_macro_instructions = {};

	// Vector which holds the current macro parameters
	std::vector< std::string > current_macro_parameters = {};

	std::vector< std::string > macro_labels = {};

	std::random_device                       rd;
	std::mt19937                             gen( rd() );
	std::uniform_int_distribution< int64_t > distrib( 0, INT64_MAX );

	// Vector which holds all of the label name's and their line index
	std::vector< std::pair< std::string, int64_t > > labels = {};

	// Needed for correct calculating the size of remove preprocessor instructions
	uint32_t empty_line_counter = 0;

	// also remove all tabs from the code lines
	// this allows the user to intendent his code, to look better
	std::ranges::for_each( code, []( auto& line )
	                       {
		                       while( line.contains( '\t' ) )
			                       line = line.substr( line.find( '\t' ) + 1, line.size() );
	                       }
	);

	// before working with the read code 
	// I delete all comments and empty lines from it
	code.erase( std::ranges::begin( std::ranges::remove_if( code, []( const auto& elem )
	                                                        {
		                                                        return elem.empty() || elem == "\t" || elem.starts_with(
			                                                        '#'
		                                                        );
	                                                        }
		            )
	            ), std::end( code )
	);

	Log::debug( "[preprocessor::process_file] Removed all empty lines and comments!\n" );

	// Check if there are any import in the code
	if( std::count_if( std::execution::par, code.begin(), code.end(), []( const auto& elem )
	                   {
		                   return elem.starts_with( "import" );
	                   }
	) > 0 )
	{
		// Iterate over the code file and resolve all imports
		for( size_t idx = 0; idx < code.size(); idx++ )
		{
			const auto& line = code.at( idx );

			// skip all unimportant lines
			if( !line.starts_with( "import" ) )
				continue;

			const auto split_line = Utils::split_string( line );

			// check if the size of the import statement is correct
			if( split_line.size() != 2 )
			{
				Log::debug( "[preprocessor::process_file] Invalid import size: %s\n", line.c_str() );

				return PREPROCESS_INVALID_IMPORT_SYNTAX;
			}

			const auto& path_to_import = split_line.back();

			// check if the imported code file exists
			if( !Utils::does_file_exists( path_to_import ) )
			{
				Log::debug( "[preprocessor::process_file] Import file does not existst: %s\n", path_to_import.c_str() );

				return PREPROCESS_IMPORT_FILE_DOES_NOT_EXIST;
			}

			auto import_code = Utils::read_file( path_to_import );

			// if the imported file is empty, just continue with the next line
			if( import_code.empty() )
			{
				Log::debug( "[preprocessor::process_file] Imported file %s was empty!\n", path_to_import.c_str() );

				continue;
			}

			// also remove all tabs from the code lines
			// this allows the user to intendent his code, to look better
			std::ranges::for_each( import_code, []( auto& line )
			                       {
				                       while( line.contains( '\t' ) )
					                       line = line.substr( line.find( '\t' ) + 1, line.size() );
			                       }
			);

			// delete all comments and empty lines from it
			import_code.erase( std::ranges::begin( std::ranges::remove_if( import_code, []( const auto& elem )
			                                                               {
				                                                               return elem.empty() || elem.starts_with(
					                                                               '#'
				                                                               );
			                                                               }
				                   )
			                   ), std::end( import_code )
			);

			// insert now the code of the imported file after the current import statement
			for( size_t jdx = 0; jdx < import_code.size(); jdx++ )
				code.insert( code.begin() + idx + jdx, import_code.at( jdx ) );

			Log::debug( "[preprocessor::process_file] Imported %lld instructions from %s\n", import_code.size(),
			            path_to_import.c_str()
			);

			// mark now the line with the import statement as removable
			code.at( idx + import_code.size() ) = PREPROCESSOR_REMOVE_LINE;

			// check now if in the imported code is also a import statement
			// this would mean this statement need also be preprocessed
			// if there is an import statement, I reset the iterator 
			// so the loop starts again from the beginning until there is no more import statement left
			if( std::ranges::find_if( import_code, []( const auto& elem )
			                          {
				                          return elem.starts_with( "import" );
			                          }
			) != import_code.end() )
			{
				Log::info( "[preprocessor::process_file] Found import statement in imported code!\n" );

				idx = -1;
			}
		}
	}

	// Check if there are any defines or macros in the code
	if( std::count_if( std::execution::par, code.begin(), code.end(), []( const auto& elem )
	                   {
		                   return elem.starts_with( "define" ) || elem.starts_with( "macro" );
	                   }
	) > 0 )
	{
		bool is_if_open = false;
		bool is_else_open = false;

		// Iterate now over the code file and extract all define instructions from it
		for( auto& line : code )
		{
			if ( line.starts_with(  "if" ) )
				is_if_open = true;

			if ( line == "else" )
				is_else_open = true;

			// if the end of an ongoing macro processing is reached
			if( line == "end" && is_macro && !is_if_open && !is_else_open )
			{
				// disable bool to indicate the finish of the macro processing
				is_macro = false;

				// push now the extracted macro to the vector
				macros.emplace_back( current_macro_name, current_macro_instructions );

				// reset now the temporary variables for name and instructions
				current_macro_name.clear();
				current_macro_instructions.clear();

				Log::debug( "[preprocessor::process_file] Added %s a new macro with %ld instructions.\n",
				            macros.back().first.c_str(), macros.back().second.size()
				);

				// mark the line as REMOVABLE
				line = PREPROCESSOR_REMOVE_LINE;

				// skip now to the next line
				continue;
			}

			if (is_if_open && line == "end")
			{
				is_if_open = false;
			}

			if (is_else_open && line == "end")
			{
				is_else_open = false;
			}

			// check if a macro is processed and add the current line to it's instructions
			// it can't be the end here because I checked for that right above here
			if( is_macro )
			{
				// if the macro has parameters
				// replace the parameter names with static ones 
				// this makes replacing these easy
				// because the preprocessor doesn't need to know their parameter names
				if( !current_macro_parameters.empty() )
				{
					Log::debug( "[preprocessor::process_file] Searching for parameters from %s in line: %s\n",
					            current_macro_name.c_str(), line.c_str()
					);

					for( size_t idx = 0; idx < current_macro_parameters.size(); idx++ )
					{
						// Search for used parameter in code line
						if( const auto ret = std::ranges::find_if( current_macro_parameters,
						                                           [&line]( const auto& param )
						                                           {
							                                           return line.contains( param );
						                                           }
						); ret != current_macro_parameters.end() )
						{
							const auto repl_marker = this->make_macro_parameter_marker(
								std::ranges::find( current_macro_parameters, ret->c_str() ) - current_macro_parameters.
								begin()
							);

							const auto reg_ex = std::vformat( "\\b{}\\b", std::make_format_args( ret->c_str() ) );

							// replace param with static marker to be replaced
							line = regex_replace( line, std::regex( reg_ex ), repl_marker );

							Log::debug( "[preprocessor::process_file] Replaced parameter in line with marker %s\n",
							            repl_marker.c_str()
							);
						}
					}
				}

				// before adding the instruction remove all tabs etc. from the line
				line = std::regex_replace( line, std::regex( "\t" ), "" );

				// append current instruction for the macro
				current_macro_instructions.push_back( line );

				Log::debug( "[preprocessor::process_file] Macro %s added instruction %s\n", current_macro_name.c_str(),
				            line.c_str()
				);

				// mark the line as REMOVABLE
				line = PREPROCESSOR_REMOVE_LINE;

				// skip to the next line
				continue;
			}

			// split now the content of the line
			const auto split_line = Utils::split_string( line );

			// get now the first string, which is the mnemonic of the line
			auto mnemonic = split_line.front();
			Utils::string_to_lower( mnemonic );

			// if the line starts with define, it is a replacement instruction
			// syntax: define <original>, <replacement>
			if( mnemonic == "define" )
			{
				// Make sure that the comma seperator is also present in the current define line
				// Also make sure that only comma exists in the define instruction
				if( !line.contains( ',' ) || std::ranges::count( line, ',' ) != 1 )
				{
					Log::debug( "[preprocessor::process_file] Detected invalid define instruction: %s\n", line.c_str()
					);
					return PREPROCESS_INVALID_DEFINE_SYNTAX;
				}

				auto orig = std::string();

				// Join now the substrings of the original
				for( size_t idx = 1; idx < split_line.size(); idx++ )
					if( !split_line.at( idx ).contains( ',' ) )
						orig += split_line.at( idx ) + " ";
					else
					{
						orig += split_line.at( idx ).substr( 0, split_line.at( idx ).length() - 1 );
						break;
					}

				Log::debug( "[preprocessor::process_file] Extracted original from define instruction: %s\n",
				            orig.c_str()
				);

				// Get now the index of the first character after the comma
				// This is important for removing the whitespace savely if the user typed a whitespace after the comma
				const auto comma_pos = line.find_last_of( ',' );
				const auto char_pos  = line.find_last_of( ' ', comma_pos + 1 );

				// Extract now the replacement from the line
				auto repl = line.substr( char_pos + 1, line.length() );

				// I can savely remove all whitespace from the replacement because if the

				Log::debug( "[preprocessor::process_file] Extracted replacement from define instruction: %s\n",
				            repl.c_str()
				);

				// Add now the original and replacement to the vector
				// modern cpp love right here
				inst_alias.emplace_back( orig, repl );

				Log::debug( "[preprocessor::process_file] Detected define instruction in [%s]\n", line.c_str() );

				// mark the line as REMOVABLE
				line = PREPROCESSOR_REMOVE_LINE;

				// go to next code line
				continue;
			}

			// if the line starts with macro, it's a definition of a macro
			// syntax: macro <name> <params>
			//         end
			if( mnemonic == "macro" )
			{
				// if the line has only two elements, then it's a macro without parameter
				if( split_line.size() == 2 )
				{
					// extract now the macro name from the line
					current_macro_name = split_line.back();

					// enable the is_macro bool, to indicate a macro is processed
					is_macro = true;

					// mark the line as REMOVABLE
					line = PREPROCESSOR_REMOVE_LINE;

					// go on to the next line
					continue;
				}

				// macro with parameters
				if( split_line.size() > 2 )
				{
					// The count of parameter is easily calculated by subtracting 
					// the macro instruction and name from the size of the splitted line 
					const auto param_count = split_line.size() - 2;

					Log::debug( "[preprocessor::process_file] Detected macro with %ld parameter.\n", param_count );

					// extract now the macro name from the line
					current_macro_name = split_line.at( 1 );

					// extract now the parameters from the line
					current_macro_parameters = { split_line.begin() + 2, split_line.end() };

					// enable the is_macro bool, to indicate a macro is processed
					is_macro = true;

					// mark the line as REMOVABLE
					line = PREPROCESSOR_REMOVE_LINE;

					// go to the next line
					continue;
				}

				Log::debug( "[preprocessor::process_file] Invalid macro syntax found in line: %s\n", line.c_str() );

				// no case matched? invalid syntax for sure
				return PREPROCESS_INVALID_MACRO_SYNTAX;
			}
		}
	}

	// Make sure no macro is still in processing, which would mean there was no end instruction found for the macro definition
	if( is_macro )
	{
		Log::debug( "[preprocessor::process_file] Missing an end instruction for macro %s.\n",
		            current_macro_name.c_str()
		);

		return PREPROCESS_MACRO_END_MISSING;
	}

	// take the code size before removing preprocessor instructions
	const auto before_size = code.size();

	// Before inserting the macros and replacing the defines
	// Delete every preprocessor specific instructions from the code
	// Delete also every empty line here
	std::erase_if( code, []( const auto& line )
	               {
		               return line == PREPROCESSOR_REMOVE_LINE || line.empty() || line == "\t";
	               }
	);

	Log::debug( "[preprocessor::process_file] Removed %ld preprocessor instructions from the code.\n",
	            before_size - code.size() - empty_line_counter
	);

	if( !macros.empty() )
	{
		// Iterate now the second time over the file and insert all macros
		for( size_t idx = 0; idx < code.size(); idx++ )
		{
			const auto& line = code.at( idx );

			// Split now the instruction
			auto split_line = Utils::split_string( line );

			// get now the mnemonic and convert it to lower case
			auto mnemonic = split_line.front();
			Utils::string_to_lower( mnemonic );

			// check now if the mnemonic is a macro
			if( auto it = std::ranges::find_if( macros, [&mnemonic]( const auto& current_macro )
			                                    {
				                                    return current_macro.first == mnemonic;
			                                    }
			); it != macros.end() )
			{
				Log::debug( "[preprocessor::process_file] Found a used macro in line %s\n", line.c_str() );

				// Check if there are any labels in the macro
				if( std::ranges::count_if( it->second, []( const auto& elem )
				                           {
					                           return elem.starts_with( "@" );
				                           }
				) > 0 )
				{
					std::string label_name = {};

					// Randomize now every label in the macro to make sure
					// that multiple uses of the macro doesn't use the same labels
					// make sure no duplicate labels are used for the current macro
					std::ranges::for_each( it->second, [&]( auto& elem )
					                       {
						                       if( elem.starts_with( '@' ) )
						                       {
							                       // erase old numbers from other iterations if macro was already randomized before
							                       elem.erase( std::remove_if( elem.begin(), elem.end(), ::isdigit ),
							                                   elem.end()
							                       );

							                       label_name = elem + std::to_string( distrib( gen ) );


							                       while( std::ranges::count_if( macro_labels, [&]( const auto& elem2 )
								                       {
									                       return elem2 == label_name;
								                       }
							                       ) > 0 )
							                       {
								                       label_name = elem + std::to_string( distrib( gen ) );
							                       }

							                       macro_labels.emplace_back( label_name );

							                       const auto pure_label_name = elem.substr( 1, elem.size() );

							                       elem = label_name;

							                       auto label_without_at = label_name.substr( 1, label_name.size() );

							                       // replace now all jmp's inside the macro with the randomized label name
							                       std::ranges::for_each( it->second, [&]( auto& elem3 )
							                                              {
								                                              if( elem3.starts_with( 'j' ) && elem3.
									                                              contains( pure_label_name ) )
								                                              {
									                                              auto split = Utils::split_string(
										                                              elem3
									                                              );

									                                              elem3 = split.front() + " " +
										                                              label_without_at;
								                                              }
							                                              }
							                       );
						                       }
					                       }
					);

					Log::debug( "[preprocessor::process_file] Randomized all used labels in macro: %s\n",
					            it->first.c_str()
					);
				}
				// if only the macro name is in the line
				// this is a macro with no parameters
				if( split_line.size() == 1 )
				{
					// Replace now the current line with the macro instructions

					// Loop through all macro instructions
					// For the first instruction overwrite the current code line with the first macro instruction
					// Insert after the remaining instructions of the macro
					for( size_t ydx = 0; ydx < it->second.size(); ydx++ )
					{
						if( ydx == 0 )
							code.at( idx ) = it->second.at( ydx );
						else
							code.insert( code.begin() + idx + ydx, it->second.at( ydx ) );
					}

					Log::debug( "[preprocessor::process_file] Inserted macro %s with %ld instructions.\n",
					            it->first.c_str(), it->second.size()
					);

					// go on to the next line
					continue;
				}

				// if the splitted line has a size greater than 1
				// the macro has parameters
				// which need to be replaced with the insertion from the user using the macro
				if( split_line.size() > 1 )
				{
					Log::debug( "[preprocessor::process_file] Detected use of macro with parameters: %s\n",
					            split_line.front().c_str()
					);

					// extract now the used parameters from the code line
					std::vector< std::string > used_params = { split_line.begin() + 1, split_line.end() };

					// Replace now the current line with the macro instructions

					// Loop through all macro instructions
					// For the first instruction overwrite the current code line with the first macro instruction
					// Insert after the remaining instructions of the macro
					for( size_t ydx = 0; ydx < it->second.size(); ydx++ )
					{
						auto macro_line = it->second.at( ydx );

						// search now in the current macro line for the static marker
						// and replace them with the parameters from the user
						for( size_t mdx = 0; mdx < used_params.size(); mdx++ )
						{
							const auto& current_marker = this->make_macro_parameter_marker( mdx );

							if( macro_line.contains( current_marker ) )
							{
								macro_line = std::regex_replace( macro_line, std::regex( current_marker ),
								                                 used_params.at( mdx )
								);

								Log::debug( "[preprocessor::process_file] Replaced marker %s in macro line: %s\n",
								            current_marker.c_str(), macro_line.c_str()
								);
							}
						}

						if( ydx == 0 )
							code.at( idx ) = macro_line;
						else
							code.insert( code.begin() + idx + ydx, macro_line );
					}

					Log::debug( "[preprocessor::process_file] Inserted macro %s with %ld instructions.\n",
					            it->first.c_str(), it->second.size()
					);
				}
			}
		}
	}

	// Iterate now over the code file again and replace all the defined originals
	if( !inst_alias.empty() )
	{
		Log::debug( "[preprocessor::process_file] Starting now the replacement with %lld defines.\n", inst_alias.size()
		);

		for( auto& line : code )
		{
			Log::debug( "[preprocessor::process_file] Searchin for replacements in: %s\n", line.c_str() );

			// Split now the instruction
			auto split_line = Utils::split_string( line );

			// get now the mnemonic and convert it to lower case
			auto mnemonic = split_line.front();
			Utils::string_to_lower( mnemonic );

			// Check if the current instruction is define, if so skip it
			if( mnemonic == "define" )
				continue;

			// Check now if something in the current instruction should be replaced
			for( const auto& elem : split_line )
			{
				// Checks if in the current instruction a replacement was used
				if( auto it = std::ranges::find_if( inst_alias, [&elem]( const auto& define )
				                                    {
					                                    // need to be sure to cover the case if the elem got an comma after it, so it will be recognized
					                                    if( elem.ends_with( ',' ) )
						                                    return std::get< 1 >( define ) == elem.substr(
							                                    0, elem.length() - 1
						                                    );

					                                    return std::get< 1 >( define ) == elem;
				                                    }
				); it != inst_alias.end() )
				{
					Log::debug( "[preprocessor::process_file] Found instruction which uses replacement: [%s]\n",
					            line.c_str()
					);

					// In this instruction was something found, which should be replaced
					line = std::regex_replace( line, std::regex( it->second ), it->first );

					Log::debug( "[preprocessor::process_file] Replaced %s with original %s\n", it->second.c_str(),
					            it->first.c_str()
					);
				}
			}
		}
	}
	else
	{
		Log::debug( "[preprocessor::process_file] No defines in code, so skipping replacement.\n" );
	}

	// needed for covering the invalid label case 
	EStatusCode temp = PARSER_SUCCESS;

	// Check if there are any labels in the code
	if( std::count_if( std::execution::par, code.begin(), code.end(), []( const auto& elem )
	                   {
		                   return elem.starts_with( "@" );
	                   }
	) > 0 )
	{
		// Search for all labels in the code and add them to the labels vector
		std::ranges::for_each( code, [&]( const auto& line )
		                       {
			                       if( line.starts_with( '@' ) )
			                       {
				                       if( line.size() == 1 )
				                       {
					                       Log::debug(
						                       "[preprocessor::process_file] Detected invalid label with no name!\n"
					                       );

					                       temp = PREPROCESS_INVALID_LABEL_SYNTAX;

					                       return;
				                       }

				                       const auto& label_name = line.substr( 1, line.size() );

				                       const auto label_line_index = static_cast< size_t >( &line - &code[ 0 ] );

				                       // add label now to the vector
				                       labels.emplace_back( label_name, label_line_index );

				                       code.erase( std::ranges::find( code, line ) );
				                       Log::debug(
					                       "[preprocessor::process_file] Added label %s with line index: %lld\n",
					                       label_name.c_str(),
					                       label_line_index
				                       );
			                       }
		                       }
		);
	}

	if( temp != PARSER_SUCCESS )
		return temp;

	// Check if there are any jmps in the code
	if( std::count_if( std::execution::par, code.begin(), code.end(), []( const auto& elem )
	                   {
		                   return elem.starts_with( "jmp" ) || elem.starts_with( "je" ) || elem.starts_with( "jg" ) ||
			                   elem.
			                   starts_with( "jl" ) || elem.starts_with( "jne" ) || elem.starts_with( "jge" ) || elem.
			                   starts_with( "jle" );
	                   }
	) > 0 )
	{
		// Iterate now over the code file to resolve all JMP's
		// if must resolve them now because otherwise the line index
		// saved in the labels vector won't match anymore
		for( size_t idx = 0; idx < code.size(); idx++ )
		{
			auto& line = code.at( idx );

			const auto split_line = Utils::split_string( line );

			const auto& mnemonic = split_line.front();

			if( mnemonic == "jmp" || mnemonic == "je" || mnemonic == "jg" || mnemonic == "jl" || mnemonic == "jne" ||
				mnemonic == "jge" || mnemonic == "jle" )
			{
				if( split_line.size() != 2 )
				{
					Log::debug( "[preprocessor::process_file] Detected invalid jmp: %s\n", line.c_str() );

					return PREPROCESS_INVALID_JMP_SYNTAX;
				}

				const auto& jmp_to_label = split_line.at( 1 );

				// search if the label is in the vector, which must be always the case
				// can't jump to somewhere if I don't know where lulz
				if( const auto ret = std::ranges::find_if( labels, [&jmp_to_label]( const auto& elem )
				                                           {
					                                           return elem.first == jmp_to_label;
				                                           }
				); ret != labels.end() )
				{
					// if something got found replace the label name with the delta from the line index
					// need to make the index signed, because forward and backwards jumps are allowed
					const auto signed_idx = static_cast< int64_t >( idx );

					// need to multiply by -1 if the signed_idx is greater than ret->second
					// this fixes backward jumps
					const auto delta = std::vformat( "{:d}", std::make_format_args(
						                                 signed_idx > ret->second
							                                 ? ( signed_idx - ret->second ) * -1
							                                 : ret->second - signed_idx
					                                 )
					);
					line = std::regex_replace( line, std::regex( jmp_to_label ), delta );
				}
			}
		}
	}

	// Now I resolve all if- and else-clauses
	// I split this process into three steps
	// 1. Validate the syntax of every if and else (more correctly: validate that an END statement to every clause is present)
	// 2. Resolving every single or nested if-clause, also inside of else-clauses, and else-clause
	// 3. Remove all unnecessary lines (else, end)

	// Validate that every if and else instruction has exactly one end statement

	const auto num_if_clauses = std::ranges::count_if( code, []( const auto& elem )
	                                                   {
		                                                   return elem.starts_with( "if" );
	                                                   }
	);
	const auto num_else_clauses = std::ranges::count_if( code, []( const auto& elem )
	                                                     {
		                                                     return elem.starts_with( "else" );
	                                                     }
	);
	const auto num_end_clauses = std::ranges::count_if( code, []( const auto& elem )
	                                                    {
		                                                    return elem == "end";
	                                                    }
	);

	Log::debug( "[preprocessor::process_file] Counted %lld if-clauses, %lld else-clauses and %lld end-statements.\n",
	            num_if_clauses, num_else_clauses, num_end_clauses
	);

	if( const auto sum = num_if_clauses + num_else_clauses; sum - num_end_clauses > 0 )
	{
		Log::debug( "[preprocessor::process_file] Detected more if- and else-clauses than end-statements!\n" );

		return PREPROCESS_IF_END_MISSING;
	}
	else if( sum - num_end_clauses < 0 )
	{
		Log::debug( "[preprocessor::process_file] Detected more end-statements than if- and else-clauses!\n" );

		return PREPROCESS_IF_END_MISSING;
	}

	Log::debug( "[preprocessor::process_file] All if- and else-clauses have exactly one end statment.\n" );

	// Resolving now all if- and else-clauses

	// Calculate now the maximum depth of if-clauses inside the code file

	// This would be so much easier but msvc don't support it yet -.- (https://en.cppreference.com/w/cpp/algorithm/ranges/find_last#top C++23)
	// const auto if_depth = std::ranges::find_last_if(code, [](const auto& elem) { return elem.starts_with("if"); });
	// Well just compensate it with some nice treats from c++20

	auto last_if_it = std::ranges::find_if(std::views::reverse(code), []( const auto& elem )
	                                        {
		                                        return elem.starts_with( "if" );
	                                        }
	).base();

	auto last_if_idx = last_if_it - code.begin() - 1;

	if( last_if_idx == -1 )
	{
		Log::debug( "[preprocessor::process_file] No if in code file was found!\n" );
	}
	else
	{
		Log::debug( "[preprocessor::process_file] Innerst if-clause on index: %lld\n", last_if_idx );

		auto if_depth = std::count_if( code.begin(),
		                               last_if_idx > 0 ? code.begin() + last_if_idx - 1 : code.begin() + last_if_idx,
		                               []( const auto& elem )
		                               {
			                               return elem.starts_with( "if" );
		                               }
		);

		Log::debug( "[preprocessor::process_file] Initial if-depth is: %lld\n", if_depth );

		while( true )
		{
			size_t if_length = 0;

			const auto num_if_clauses2 = std::ranges::count_if( code, []( const auto& elem )
			                                                    {
				                                                    return elem.starts_with( "if" );
			                                                    }
			);
			const auto num_else_clauses2 = std::ranges::count_if( code, []( const auto& elem )
			                                                      {
				                                                      return elem.starts_with( "else" );
			                                                      }
			);
			const auto num_end_clauses2 = std::ranges::count_if( code, []( const auto& elem )
			                                                     {
				                                                     return elem == "end";
			                                                     }
			);

			Log::debug(
				"[preprocessor::process_file] Counted %lld if-clauses, %lld else-clauses and %lld end-statements.\n",
				num_if_clauses2, num_else_clauses2, num_end_clauses2
			);

			// calculate if-clause length
			for( size_t idx = last_if_idx; idx < code.size(); idx++ )
			{
				// until the end of the if-clause, increase the length for every instruction
				if( code.at( idx ) != "end" )
				{
					if_length++;

					continue;
				}

				// now is the end of the if-clause reached

				size_t else_length = 0;

				// check now if after the end an else is followed
				if( idx + 1 < code.size() && code.at( idx + 1 ) == "else" )
				{
					Log::debug( "[preprocessor::process_file] Found else with index: %lld from if: %s\n", idx + 1,
					            code.at( last_if_idx ).c_str()
					);

					// calculate now the length of the else-clause
					for( size_t jdx = idx + 1; jdx < code.size(); jdx++ )
					{
						// until the end of the else-clause, increase the length for every instruction
						if( code.at( jdx ) != "end" )
						{
							else_length++;

							continue;
						}

						// end of else-clause is reached
						Log::debug( "[preprocessor::process_file] Removing else: %lld -> %s\n", idx + 1,
						            code.at( idx + 1 ).c_str()
						);
						// remove else clause
						// else
						code.erase( code.begin() + idx + 1 );

						Log::debug( "[preprocessor::process_file] Removing end: %lld -> %s\n", jdx - 1,
						            code.at( jdx - 1 ).c_str()
						);
						// end
						code.erase( code.begin() + jdx - 1 );

						if( else_length > 1 )
						{
							// overwrite now the end of the if-clause with an jmp and the else-length
							code.at( idx ) = std::vformat( "jmp {:d}", std::make_format_args( else_length ) );
						}
						else
						{
							// remove end of if-clause
							code.erase( code.begin() + idx );
						}

						break;
					}

					// append now to the if-clause the length
					code.at( last_if_idx ) += std::vformat( " {:d}", std::make_format_args( if_length ) );
				}
				else
				{
					// if there is no else after the end of the if-clause

					// empty if-clause
					if( if_length == 1 )
					{
						Log::debug( "[preprocessor::process_file] Empty if-clause found with line: %s\n",
						            code.at( idx ).c_str()
						);
						// delete it
						// if
						code.erase( code.begin() + idx - 1 );

						//end
						code.erase( code.begin() + idx - 1 );
					}
					else
					{
						// append now to the if-clause the length
						code.at( last_if_idx ) += std::vformat( " {:d}", std::make_format_args( if_length - 1 ) );

						// remove end from if-clause
						code.erase( code.begin() + last_if_idx + if_length );
					}

					break;
				}

				// empty if-clause
				if( if_length == 1 && else_length == 0 )
				{
					Log::debug( "[preprocessor::process_file] Empty if-clause found with line: %s\n",
					            code.at( idx ).c_str()
					);

					// delete it
					// if
					Log::debug( "[preprocessor::process_file] Removing if-instruction: %lld -> %s\n", idx - 1,
					            code.at( idx - 1 ).c_str()
					);
					code.erase( code.begin() + idx - 1 );
				}

				break;
			}

			auto code_reverse = std::views::reverse( code );

			last_if_it = std::ranges::find_if( code_reverse, []( const auto& elem )
			                                   {
				                                   return elem.starts_with( "if" ) && Utils::split_string( elem ).size()
					                                   == 5;
			                                   }
			).base();

			last_if_idx = last_if_it - code.begin() - 1;

			if( last_if_idx == -1 )
				break;

			if_depth = std::count_if( code.begin(), code.begin() + last_if_idx, []( const auto& elem )
			                          {
				                          return elem.starts_with( "if" );
			                          }
			) - 1;

			if( if_depth == -1 && last_if_idx == -1 )
				break;

			Log::debug( "[preprocessor::process_file] Next if-clause is at index: %lld\n", last_if_idx );

			Log::debug( "[preprocessor::process_file] Next if-depth is: %lld\n", if_depth );
		}
	}

	// .prp file name
	const auto prp_name = std::string( path_to_file.substr( 0, path_to_file.find_last_of( '.' ) ) + ".prp" );

	// After the preprocessing is done, write now the preprocessor file to disk
	if( !Utils::write_as_file( prp_name, code ) )
		return PREPROCESS_COULD_NOT_WRITE_FILE;

	Log::debug( "[preprocessor::process_file] Wrote preprocessed file %s to disk!\n", prp_name.c_str() );

	return PREPROCESS_SUCCESS;
}
