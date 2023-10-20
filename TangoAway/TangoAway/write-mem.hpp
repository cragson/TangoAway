#pragma once
#include <memory>

#include "instruction.hpp"

class inst_write_mem : public instruction
{
public:
	inst_write_mem( const std::string& data, const std::string& address, const std::string& value )
		: m_datatype{ data }
		, m_address{ address }
		, m_value{ value } {}

	void evaluate( std::shared_ptr< environment > env ) override;

private:
	std::string m_datatype, m_address, m_value;
};
