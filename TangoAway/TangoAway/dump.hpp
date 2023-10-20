#pragma once
#include <memory>

#include "instruction.hpp"

class inst_dump : public instruction
{
public:
	inst_dump( const std::string& address, const std::string& size, const std::string& file_name )
		: m_address{ address }
		, m_size{ size }
		, m_output_file_name{ file_name } {}

	void evaluate(std::shared_ptr<environment> env) override;

private:
	std::string m_address, m_size, m_output_file_name;
};
