#include "show_threads_info.hpp"
#include "windows_api.hpp"


void inst_show_threads_info::evaluate( std::shared_ptr< environment > env )
{
	TangoAway_WindowsAPI::print_threads_info( env );
}
