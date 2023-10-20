#pragma once
#include <memory>

#include "instruction.hpp"

class inst_read_mem_imm : public instruction
{
public:
	inst_read_mem_imm( const std::string& datatype, const std::string& addr, const std::string& vd )
		: m_datatype{ datatype }
		, m_address_to_read{ addr }
		, m_vdr{ vd } {}

	void evaluate( std::shared_ptr< environment > env ) override;

private:
	std::string m_datatype, m_address_to_read, m_vdr;
};
