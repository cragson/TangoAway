#include "find_signature.hpp"

#include "windows_api.hpp"

void inst_find_signature::evaluate( std::shared_ptr< environment > env )
{
	const auto ret = TangoAway_WindowsAPI::find_signature( env, this->m_name, this->m_sig );

	env->write_to_vdr< std::uintptr_t >( this->m_vdr_ident, ret );
}
