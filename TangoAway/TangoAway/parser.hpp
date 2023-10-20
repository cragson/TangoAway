#pragma once
#include <any>
#include <functional>
#include <memory>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

#include "instruction.hpp"
#include "status_code.hpp"

class parser
{
public:
	parser();

	[[nodiscard]] static EStatusCode is_valid_two_part_instruction( const std::string& inst );

	//[[nodiscard]] static EStatusCode is_valid_four_part_instruction(const std::string& inst);

	[[nodiscard]] EStatusCode parse_file( const std::string& path_to_file );

	[[nodiscard]] EStatusCode parse_file_multithreaded_chunking( const std::string& path_to_file,
	                                                             const size_t       thread_count
	);

	[[nodiscard]] EStatusCode parse_file_multithreaded_chunking( const std::string& path_to_file );

	[[nodiscard]] auto get_instructions() noexcept
	{
		return &this->m_instructions;
	}

	[[nodiscard]] auto has_instructions() const noexcept
	{
		return !this->m_instructions.empty();
	}

	[[nodiscard]] auto get_instructions_size() const noexcept
	{
		return this->m_instructions.size();
	}

	[[nodiscard]] auto get_instruction_by_index( const size_t idx )
	{
		return this->m_instructions.at( idx );
	}

	inline void clear_instructions()
	{
		this->m_instructions.clear();
	}

	[[nodiscard]] inline static auto is_vdr( const std::string& input )
	{
		static std::regex vdr_regex( "vdr(\\d|1[0-5]{1})" );

		return std::regex_match( input, vdr_regex ) ? true : false;
	}

	static constexpr auto is_datatype( const std::string& input )
	{
		if( input == "byte" || input == "word" || input == "dword" || input == "qword" || input == "float" )
			return true;

		return false;
	}

	[[nodiscard]] inline static auto is_immediate( const std::string& input )
	{
		static std::regex imm_regex( "(0[xX]([a-fA-F0-9]){1,16})|(\\d+)|\\d+\\.\\d+" );

		return std::regex_match( input, imm_regex );
	}

	static constexpr auto is_printtype( const std::string& type )
	{
		return is_datatype( type ) || type == "hex32" || type == "hex64" || type == "ascii";
	}


	static constexpr auto is_if_operator( const std::string& op )
	{
		return op == "==" || op == "!=" || op == ">" || op == ">=" || op == "<" || op == "<=";
	}

private:
	std::vector< std::shared_ptr< instruction > > m_instructions;
};
