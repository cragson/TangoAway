#include "terminate_process.hpp"

#include "parser.hpp"

void inst_terminate_process::evaluate( std::shared_ptr< environment > env )
{
	std::uintptr_t pid = {};

	if( parser::is_vdr( this->m_pid ) )
	{
		pid = env->read_from_vdr( vdr::vdr_str_to_enum( this->m_pid ) )->vdr_.vdr64;
	}
	else
	{
		// addres is an immediate, so I convert it easily
		if( this->m_pid.starts_with( "0x" ) )
			pid = std::stoi( this->m_pid, nullptr, 16 );
		else
			pid = std::stoi( this->m_pid );
	}

	const auto handle = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );

	if( !handle )
		return;

	TerminateProcess( handle, 0 );

	CloseHandle( handle );
}
