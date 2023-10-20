#pragma once

#include "instruction.hpp"

class inst_dec_vdr : public instruction
{
public:
	inst_dec_vdr( const EVdrIdentifier& ident )
	{
		this->m_vdr = ident;
	}

	void evaluate(std::shared_ptr< environment > env ) override
	{
		env->write_to_vdr< std::uintptr_t >( this->m_vdr, env->read_from_vdr( this->m_vdr )->vdr_.vdr64 - 1 );
	}

private:
	EVdrIdentifier m_vdr;
};
