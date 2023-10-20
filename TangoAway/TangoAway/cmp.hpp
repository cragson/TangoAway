#pragma once
#include <memory>

#include "instruction.hpp"

class inst_cmp : public instruction
{
public:
	explicit inst_cmp( const std::string& op1, const std::string& op2 )
		: m_operand1{ op1 }
		, m_operand2{ op2 } {}

	void evaluate(std::shared_ptr< environment > env) override;

private:
	std::string m_operand1;
	std::string m_operand2;
};
