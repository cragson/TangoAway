#include "write-mem.hpp"

#include "parser.hpp"

#include "windows_api.hpp"

void inst_write_mem::evaluate( std::shared_ptr< environment > env )
{
	std::uintptr_t addr = {};

	if( parser::is_vdr( this->m_address ) )
	{
		addr = env->read_from_vdr( vdr::vdr_str_to_enum( this->m_address ) )->vdr_.vdr64;
	}
	else
	{
		// address is an immediate, so I convert it easily
		if( this->m_address.starts_with( "0x" ) )
			addr = std::stoll( this->m_address, nullptr, 16 );
		else
			addr = std::stoll( this->m_address );
	}

	const auto is_value_vdr = parser::is_vdr( this->m_value );

	if( this->m_datatype == "byte" )
	{
		TangoAway_WindowsAPI::write_memory< uint8_t >( env, addr, is_value_vdr
			                                                          ? env->read_from_vdr(
				                                                          vdr::vdr_str_to_enum( this->m_value )
			                                                          )->vdr_.vdr8
			                                                          : std::stoul( this->m_value )
		);

		return;
	}
	if( this->m_datatype == "word" )
	{
		TangoAway_WindowsAPI::write_memory< uint16_t >( env, addr, is_value_vdr
			                                                           ? env->read_from_vdr(
				                                                           vdr::vdr_str_to_enum( this->m_value )
			                                                           )->vdr_.vdr16
			                                                           : std::stoul( this->m_value )
		);

		return;
	}
	if( this->m_datatype == "dword" )
	{
		TangoAway_WindowsAPI::write_memory< uint32_t >( env, addr, is_value_vdr
			                                                           ? env->read_from_vdr(
				                                                           vdr::vdr_str_to_enum( this->m_value )
			                                                           )->vdr_.vdr32
			                                                           : std::stoul( this->m_value )
		);

		return;
	}
	if( this->m_datatype == "qword" )
	{
		TangoAway_WindowsAPI::write_memory< uint64_t >( env, addr, is_value_vdr
			                                                           ? env->read_from_vdr(
				                                                           vdr::vdr_str_to_enum( this->m_value )
			                                                           )->vdr_.vdr64
			                                                           : std::stoul( this->m_value )
		);

		return;
	}
	if( this->m_datatype == "float" )
	{
		TangoAway_WindowsAPI::write_memory< float >( env, addr, is_value_vdr
			                                                        ? env->read_from_vdr(
				                                                        vdr::vdr_str_to_enum( this->m_value )
			                                                        )->vdr_.vdr_fl
			                                                        : std::stof( this->m_value )
		);

		return;
	}
}
