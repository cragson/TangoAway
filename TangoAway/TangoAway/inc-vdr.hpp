#pragma once
#include "instruction.hpp"

#include "vdr.hpp"

class inst_inc_vdr : public instruction
{
public:
	inst_inc_vdr() = delete;

	explicit inst_inc_vdr( const EVdrIdentifier ident )
	{
		this->m_vdr = ident;
	}

	void evaluate( std::shared_ptr< environment > env ) override
	{
		env->write_to_vdr< std::uintptr_t >( m_vdr, env->read_from_vdr( m_vdr )->vdr_.vdr64 + 1 );
	}

private:
	EVdrIdentifier m_vdr;
};
