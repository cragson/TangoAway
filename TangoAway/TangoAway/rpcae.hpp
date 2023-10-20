#pragma once
#include <limits>
#include <memory>

#include "instruction.hpp"

class inst_rpcae : public instruction
{
public:

	void evaluate(std::shared_ptr<environment> env) override
	{
		constexpr auto max_limit = (std::numeric_limits< size_t >::max)();
		env->set_pc( max_limit );
	}
};