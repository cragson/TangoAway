#pragma once
#include <memory>
#include <string>

#include "instruction.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

class inst_sleep_ms_vdr : public instruction
{
public:
	inst_sleep_ms_vdr( const EVdrIdentifier ident ) : m_vdr{ ident } {}

	void evaluate( std::shared_ptr< environment > env ) override
	{
		Sleep( env->read_from_vdr( this->m_vdr )->vdr_.vdr32 );
	}

private:
	EVdrIdentifier m_vdr;
};
