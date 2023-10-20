#include "resume_process.hpp"
#include "windows_api.hpp"

void inst_resume_process::evaluate( std::shared_ptr< environment > env )
{
	TangoAway_WindowsAPI::resume_process( env );
}
