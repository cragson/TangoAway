#include "show_images_info.hpp"
#include "windows_api.hpp"

void inst_show_images_info::evaluate( std::shared_ptr< environment > env )
{
	TangoAway_WindowsAPI::print_images_info( env );
}
