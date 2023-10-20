#pragma once
#include <memory>

#include "instruction.hpp"

class inst_terminate_process : public instruction
{
public:
	inst_terminate_process() = delete;

	inst_terminate_process( const std::string& pid ) : m_pid{ pid } {}

	void evaluate( std::shared_ptr< environment > env ) override;

private:
	std::string m_pid;
};
