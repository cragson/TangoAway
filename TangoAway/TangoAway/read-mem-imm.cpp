#include "read-mem-imm.hpp"

#include <string>

#include "windows_api.hpp"

void inst_read_mem_imm::evaluate( std::shared_ptr< environment > env )
{
	const auto addr = std::stoll(this->m_address_to_read);

	const auto vdr_ident = vdr::vdr_str_to_enum(this->m_vdr);

	if (this->m_datatype == "byte")
	{
		const auto ret = TangoAway_WindowsAPI::read_memory< uint8_t >(env, addr);

		env->write_to_vdr< uint8_t >(vdr_ident, ret);
	}

	else if (this->m_datatype == "word")
	{
		const auto ret = TangoAway_WindowsAPI::read_memory< uint16_t >(env, addr);

		env->write_to_vdr< uint16_t >(vdr_ident, ret);
	}

	else if (this->m_datatype == "dword")
	{
		const auto ret = TangoAway_WindowsAPI::read_memory< uint32_t >(env, addr);

		env->write_to_vdr< uint32_t >(vdr_ident, ret);
	}

	else if (this->m_datatype == "qword")
	{
		const auto ret = TangoAway_WindowsAPI::read_memory< uint64_t >(env, addr);

		env->write_to_vdr< uint64_t >(vdr_ident, ret);
	}

	else if (this->m_datatype == "float")
	{
		const auto ret = TangoAway_WindowsAPI::read_memory< float >(env, addr);

		env->write_to_vdr< float >(vdr_ident, ret);
	}
}
