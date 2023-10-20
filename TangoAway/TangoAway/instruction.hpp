#pragma once
#include "environment.hpp"

class instruction
{
public:
	virtual void evaluate( std::shared_ptr< environment > env ) = 0;
};
