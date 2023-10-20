#include "suspend_process.hpp"

#include "windows_api.hpp"

void inst_suspend_process::evaluate( std::shared_ptr< environment > env )
{
	TangoAway_WindowsAPI::suspend_process( env );
}
