#include "patch_bytes.hpp"

#include <string>

#include "parser.hpp"
#include "windows_api.hpp"

#include "logging.hpp"

void inst_patch_bytes::evaluate( std::shared_ptr< environment > env )
{
	std::uintptr_t addr = {};

	if( parser::is_vdr( this->m_address ) )
	{
		addr = env->read_from_vdr( vdr::vdr_str_to_enum( this->m_address ) )->vdr_.vdr64;
	}
	else
	{
		// addres is an immediate, so I convert it easily
		if( this->m_address.starts_with( "0x" ) )
			addr = std::stoll( this->m_address, nullptr, 16 );
		else
			addr = std::stoll( this->m_address );
	}

	std::vector< uint8_t > bytes;

	std::string Temp = {};

	// remove all whitespace from the signature
	std::erase_if( this->m_bytes, isspace );

	// Convert string pattern to byte pattern
	for( const auto& ch : this->m_bytes )
	{
		if( Temp.length() != 2 )
			Temp += ch;

		if( Temp.length() == 2 )
		{
			std::erase_if( Temp, isspace );
			auto converted_pattern_byte = strtol( Temp.c_str(), nullptr, 16 ) & 0xFFF;
			bytes.emplace_back( converted_pattern_byte );
			Temp.clear();
		}
	}

	const auto ret = TangoAway_WindowsAPI::write_protected_memory( env, addr, bytes );

	Log::debug( "[inst_patch_bytes::evaluate] %s\n", ret ? "patched bytes successfully!" : "failed to patch bytes!" );
}
