#pragma once
#include <memory>
#include <string>
#include <vector>

#include "instruction.hpp"

class inst_println : public instruction
{
public:
	inst_println() = delete;

	explicit inst_println( const std::vector< std::string >& args )
	{
		this->m_args = args;
	}

	void evaluate( std::shared_ptr< environment > env ) override
	{
		if( !env->is_printing() )
			return;

		std::string objs = {};

		std::string fmt       = {};
		std::string last_type = {};

		std::ranges::for_each( this->m_args, [&]( const auto& elem )
		                       {
			                       if( parser::is_printtype( elem ) )
			                       {
				                       if( elem == "byte" )
					                       fmt = "{:d} ";

				                       else if( elem == "word" || elem == "dword" || elem == "qword" )
					                       fmt = "{:d} ";

				                       else if( elem == "float" )
					                       fmt = "{:.4f} ";

				                       else if( elem == "hex32" )
					                       fmt = "{:08X} ";

				                       else if( elem == "hex64" )
					                       fmt = "0x{:016X} ";

				                       else if( elem == "ascii" )
					                       fmt = "%s ";

				                       last_type = elem;
			                       }

			                       if( parser::is_vdr( elem ) )
			                       {
				                       if( last_type == "byte" )
					                       objs += std::vformat(
						                       fmt, std::make_format_args(
							                       env->read_from_vdr( vdr::vdr_str_to_enum( elem ) )->vdr_.vdr8
						                       )
					                       );

				                       else if( last_type == "word" )
					                       objs += std::vformat(
						                       fmt, std::make_format_args(
							                       env->read_from_vdr( vdr::vdr_str_to_enum( elem ) )->vdr_.vdr16
						                       )
					                       );

				                       else if( last_type == "dword" )
					                       objs += std::vformat(
						                       fmt, std::make_format_args(
							                       env->read_from_vdr( vdr::vdr_str_to_enum( elem ) )->vdr_.vdr32
						                       )
					                       );


				                       else if( last_type == "qword" )
					                       objs += std::vformat(
						                       fmt, std::make_format_args(
							                       env->read_from_vdr( vdr::vdr_str_to_enum( elem ) )->vdr_.vdr64
						                       )
					                       );


				                       else if( last_type == "float" )
					                       objs += std::vformat(
						                       fmt, std::make_format_args(
							                       env->read_from_vdr( vdr::vdr_str_to_enum( elem ) )->vdr_.vdr_fl
						                       )
					                       );


				                       else if( last_type == "hex32" )
					                       objs += std::vformat(
						                       fmt, std::make_format_args(
							                       env->read_from_vdr( vdr::vdr_str_to_enum( elem ) )->vdr_.vdr32
						                       )
					                       );


				                       else if( last_type == "hex64" )
					                       objs += std::vformat(
						                       fmt, std::make_format_args(
							                       env->read_from_vdr( vdr::vdr_str_to_enum( elem ) )->vdr_.vdr64
						                       )
					                       );
				                       else if( last_type == "ascii" )
				                       {
					                       auto vdr_data = env->read_from_vdr( vdr::vdr_str_to_enum( elem ) )->vdr_.
					                                            vdr64;

					                       std::string temp = {};

					                       for( size_t idx = 0; idx < 8; idx++ )
					                       {
						                       const auto ch_ptr = reinterpret_cast< uint8_t* >( &vdr_data );
						                       const auto ch     = static_cast< char >( *( ch_ptr + idx ) );

						                       if( ch < 0 || !std::isprint( ch ) )
							                       temp += '.';
						                       else
							                       temp += ch;
					                       }

					                       objs += temp;
				                       }


				                       fmt       = {};
				                       last_type = {};
			                       }

			                       if( !parser::is_vdr( elem ) && !parser::is_printtype( elem ) )
				                       objs += elem + ' ';
		                       }
		);


		printf( "[%s] ", env->get_name().c_str() );

		printf( "%s", objs.c_str() );

		printf( "\n" );
	}

private:
	std::vector< std::string > m_args;
};
