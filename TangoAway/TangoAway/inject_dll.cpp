#include "inject_dll.hpp"

#include "windows_api.hpp"

void inst_inject_dll::evaluate( std::shared_ptr< environment > env )
{
	if( this->m_method == "loadlibrary")
	{
		TangoAway_WindowsAPI::inject_loadlibrary(env, this->m_dll_path);
	}
}
