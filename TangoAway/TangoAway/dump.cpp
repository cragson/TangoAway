#include "dump.hpp"

#include "logging.hpp"
#include "parser.hpp"
#include "utils.hpp"
#include "windows_api.hpp"

void inst_dump::evaluate( std::shared_ptr< environment > env )
{
	std::uintptr_t addr = {};

	if( parser::is_vdr(this->m_address))
	{
		addr = env->read_from_vdr(vdr::vdr_str_to_enum(this->m_address))->vdr_.vdr64;
	}
	else
	{
		addr = Utils::str_to_ul(this->m_address);
	}

	size_t size = {};

	if (parser::is_vdr(this->m_size))
	{
		size = env->read_from_vdr(vdr::vdr_str_to_enum(this->m_size))->vdr_.vdr64;
	}
	else
	{
		size = Utils::str_to_ul(this->m_size);
	}

	const auto bytes = TangoAway_WindowsAPI::read_memory_area(env, addr, size);

	if ( bytes.empty() )
	{
		Log::debug("[inst_dump::evaluate] Could not read the area from memory!\n");

		return;
	}

	Utils::write_bytes_as_file(this->m_output_file_name, bytes);
}
