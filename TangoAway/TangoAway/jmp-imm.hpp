#pragma once
#include <memory>
#include <string>

#include "instruction.hpp"

class inst_jmp_imm : public instruction
{
public:
	inst_jmp_imm( const std::string& rel ) : m_rel{ rel } { }

	void evaluate( std::shared_ptr< environment > env ) override
	{
		// convert the number
		const auto num = std::stoi( this->m_rel );

		// correct the program counter of the env by rel
		// if the rel is negative, the pc will decrease
		// if the rel is positive, the pc will increase
		// one function to handle it all
		env->set_pc( env->get_pc() + num );
	}

private:
	std::string m_rel;
};
