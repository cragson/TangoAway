#pragma once
#include <memory>

#include "instruction.hpp"

class inst_show_threads_info : public instruction
{
public:
	inst_show_threads_info() = default;

	void evaluate( std::shared_ptr< environment > env ) override;
};
