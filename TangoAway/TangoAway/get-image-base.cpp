#include "get-image-base.hpp"

#include "windows_api.hpp"

void inst_get_image_base::evaluate(std::shared_ptr<environment> env)
{
	const auto vdr_ident = vdr::vdr_str_to_enum(this->m_vdr);

	const auto ret = TangoAway_WindowsAPI::get_image_base(env, this->m_image_name);

	env->write_to_vdr< std::uintptr_t >(vdr_ident, ret);

}
