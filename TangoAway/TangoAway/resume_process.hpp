#pragma once
#include <memory>

#include "instruction.hpp"

class inst_resume_process : public instruction
{
public:
	void evaluate( std::shared_ptr< environment > env ) override;
};
