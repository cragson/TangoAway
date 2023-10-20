#pragma once
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string>
#include <vector>

namespace Utils
{
	inline auto does_file_exists( const std::string& path )
	{
		return std::filesystem::exists( path );
	}

	constexpr auto string_to_lower = []( std::string& str )
	{
		std::ranges::transform( str, str.begin(), []( auto c )
		                        {
			                        return std::tolower( c );
		                        }
		);
	};

	constexpr auto vec_to_lower = []( std::vector< std::string >& vec )
	{
		std::ranges::for_each( vec, string_to_lower );
	};

	constexpr auto get_user_input = []( const std::string& pre_msg_to_print = ">>> " )
	{
		if( !pre_msg_to_print.empty() )
			printf( "%s", pre_msg_to_print.c_str() );

		std::string ret = {};

		std::getline( std::cin, ret );

		return ret;
	};

	constexpr auto split_string = []( const std::string& user_input )
	{
		std::vector< std::string > parsed_args = {};

		if( user_input.empty() )
			return parsed_args;

		std::string parsed_input = {};

		for( const auto idx : user_input )
		{
			if( idx == ' ' )
			{
				parsed_args.push_back( parsed_input );

				parsed_input = "";
			}
			else
				parsed_input += idx;
		}

		// Need to push back one more time, to don't lose the last parsed word
		parsed_args.push_back( parsed_input );

		return parsed_args;
	};

	constexpr auto is_number( const std::string& input )
	{
		return std::ranges::find_if( input, []( const auto& ch )
		                             {
			                             return !std::isdigit( ch );
		                             }
		) == input.end();
	}

	constexpr auto read_file = []( const std::string& file_path )
	{
		std::vector< std::string > ret = {};

		std::ifstream file( file_path );

		if( !file.good() )
		{
			printf( "[!] Could not read the file: %s\n", file_path.c_str() );

			return ret;
		}

		std::string current_line = {};

		// read the file now line by line, without empty lines
		while( std::getline( file, current_line ) )
		{
			ret.push_back( current_line );
		}

		return ret;
	};

	constexpr auto write_as_file = []( const std::string& file_name, const std::vector< std::string >& content )
	{
		std::ofstream out_file( file_name );

		if( out_file.bad() )
			return false;

		std::ostream_iterator< std::string > it( out_file, "\n" );

		std::ranges::copy( content, it );

		out_file.close();

		return true;
	};


	// Credits: https://www.geeksforgeeks.org/remove-extra-spaces-string/
	inline void removeSpaces( std::string& str )
	{
		// n is length of the original string
		auto n = str.length();

		// i points to next position to be filled in
		// output string/ j points to next character
		// in the original string
		int i = 0, j = -1;

		// flag that sets to true is space is found
		bool spaceFound = false;

		// Handles leading spaces
		while( ++j < n && str[ j ] == ' ' );

		// read all characters of original string
		while( j < n )
		{
			// if current characters is non-space
			if( str[ j ] != ' ' )
			{
				// remove preceding spaces before dot,
				// comma & question mark
				if( ( str[ j ] == '.' || str[ j ] == ',' || str[ j ] == '?' ) && i - 1 >= 0 && str[ i - 1 ] == ' ' )
					str[ i - 1 ] = str[ j++ ];

				else
					// copy current character at index i
					// and increment both i and j
					str[ i++ ] = str[ j++ ];

				// set space flag to false when any
				// non-space character is found
				spaceFound = false;
			}
			// if current character is a space
			else if( str[ j++ ] == ' ' )
			{
				// If space is encountered for the first
				// time after a word, put one space in the
				// output and set space flag to true
				if( !spaceFound )
				{
					str[ i++ ] = ' ';
					spaceFound = true;
				}
			}
		}

		// Remove trailing spaces
		if( i <= 1 )
			str.erase( str.begin() + i, str.end() );
		else
			str.erase( str.begin() + i - 1, str.end() );
	}

	[[nodiscard]] inline auto str_to_ul( const std::string& input )
	{
		return input.contains( "0x" ) ? std::stoull( input, nullptr, 16 ) : std::stoull( input, nullptr, 10 );
	}

	[[nodiscard]] inline auto delete_file( const std::string& path_to_file )
	{
		return std::filesystem::remove( path_to_file );
	}

	[[nodiscard]] inline bool edit_file( const std::string& path_to_file, auto&& fn_edit, const std::string& prefix = ""
	)
	{
		if( !does_file_exists( path_to_file ) )
			return false;

		auto data = read_file( path_to_file );

		if( data.empty() )
			return false;

		std::ranges::for_each( data, fn_edit );

		if( !write_as_file( prefix.empty() ? path_to_file : prefix + path_to_file, data ) )
			return false;

		return true;
	}

	inline void write_bytes_as_file( const std::string& output_path_to_file, const std::vector< uint8_t >& bytes )
	{
		auto myfile = std::fstream( output_path_to_file, std::ios::out | std::ios::binary );
		myfile.write( ( char* )&bytes[ 0 ], bytes.size() );
		myfile.close();
	}
}
