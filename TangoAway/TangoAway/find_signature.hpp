#pragma once
#include <memory>

#include "instruction.hpp"

class inst_find_signature : public instruction
{
public:
	inst_find_signature( const EVdrIdentifier ident, const std::string& name, const std::string& sig )
		: m_vdr_ident{ ident }
		, m_name{ name }
		, m_sig{ sig } {}

	void evaluate( std::shared_ptr< environment > env ) override;

private:
	EVdrIdentifier m_vdr_ident;
	std::string    m_name, m_sig;
};
