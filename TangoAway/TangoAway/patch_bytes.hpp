#pragma once
#include <memory>

#include "instruction.hpp"

class inst_patch_bytes : public instruction
{
public:
	inst_patch_bytes( const std::string& addr, const std::string& bytes )
		: m_address{ addr }
		, m_bytes{ bytes } {}

	void evaluate( std::shared_ptr< environment > env ) override;

private:
	std::string m_address, m_bytes;
};
