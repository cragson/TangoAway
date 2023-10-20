#include "cmp.hpp"

#include "parser.hpp"

void inst_cmp::evaluate( std::shared_ptr< environment > env )
{
	int64_t op1_val = {};

	if( parser::is_vdr( this->m_operand1 ) )
		op1_val = env->read_from_vdr( vdr::vdr_str_to_enum( this->m_operand1 ) )->vdr_.vdr64s;
	else
	{
		if( this->m_operand1.starts_with( "0x" ) )
			op1_val = std::stoll( this->m_operand1, nullptr, 16 );
		else
			op1_val = std::stoll( this->m_operand1 );
	}

	int64_t op2_val = {};

	if( parser::is_vdr( this->m_operand2 ) )
		op2_val = env->read_from_vdr( vdr::vdr_str_to_enum( this->m_operand2 ) )->vdr_.vdr64s;
	else
	{
		if( this->m_operand2.starts_with( "0x" ) )
			op2_val = std::stoll( this->m_operand2, nullptr, 16 );
		else
			op2_val = std::stoll( this->m_operand2 );
	}

	// clear old flags before setting new ones
	env->clear_flags();

	const auto temp = op1_val - op2_val;

	if( temp == 0 )
		env->set_equal_flag();
	else if( temp > 0 )
		env->set_greater_flag();
	else // temp < 0
		env->set_less_flag();
}
