#pragma once
#include <functional>
#include <memory>

#include "instruction.hpp"

#include "parser.hpp"

class inst_if : public instruction
{
public:
	inst_if( const std::string& datatype, const std::string& operand1, const std::string& op,
	         const std::string& operand2, const size_t       insts_size
	)
	{
		this->m_datatype   = datatype;
		this->m_operand1   = operand1;
		this->m_op         = op;
		this->m_operand2   = operand2;
		this->m_insts_size = insts_size;
	}

	void evaluate( std::shared_ptr< environment > env ) override
	{
		const auto vdr_ident_op1 = vdr::vdr_str_to_enum( this->m_operand1 );

		const auto vdr_op1_data_ptr = env->read_from_vdr( vdr_ident_op1 );

		if( parser::is_vdr( this->m_operand2 ) )
		{
			const auto vdr_ident_op2 = vdr::vdr_str_to_enum( this->m_operand2 );

			const auto vdr_op2_data_ptr = env->read_from_vdr( vdr_ident_op2 );

			if( this->m_datatype == "byte" )
			{
				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr8s == vdr_op2_data_ptr->vdr_.vdr8s )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr8s != vdr_op2_data_ptr->vdr_.vdr8s )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr8s > vdr_op2_data_ptr->vdr_.vdr8s )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr8s >= vdr_op2_data_ptr->vdr_.vdr8s )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr8s < vdr_op2_data_ptr->vdr_.vdr8s )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr8s <= vdr_op2_data_ptr->vdr_.vdr8s )
				{
					return;
				}
			}

			else if( this->m_datatype == "word" )
			{
				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr16s == vdr_op2_data_ptr->vdr_.vdr16s )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr16s != vdr_op2_data_ptr->vdr_.vdr16s )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr16s > vdr_op2_data_ptr->vdr_.vdr16s )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr16s >= vdr_op2_data_ptr->vdr_.vdr16s )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr16s < vdr_op2_data_ptr->vdr_.vdr16s )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr16s <= vdr_op2_data_ptr->vdr_.vdr16s )
				{
					return;
				}
			}

			else if( this->m_datatype == "dword" )
			{
				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr32s == vdr_op2_data_ptr->vdr_.vdr32s )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr32s != vdr_op2_data_ptr->vdr_.vdr32s )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr32s > vdr_op2_data_ptr->vdr_.vdr32s )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr32s >= vdr_op2_data_ptr->vdr_.vdr32s )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr32s < vdr_op2_data_ptr->vdr_.vdr32s )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr32s <= vdr_op2_data_ptr->vdr_.vdr32s )
				{
					return;
				}
			}

			else if( this->m_datatype == "qword" )
			{
				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr64s == vdr_op2_data_ptr->vdr_.vdr64s )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr64s != vdr_op2_data_ptr->vdr_.vdr64s )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr64s > vdr_op2_data_ptr->vdr_.vdr64s )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr64s >= vdr_op2_data_ptr->vdr_.vdr64s )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr64s < vdr_op2_data_ptr->vdr_.vdr64s )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr64s <= vdr_op2_data_ptr->vdr_.vdr64s )
				{
					return;
				}
			}

			else if( this->m_datatype == "float" )
			{
				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr_fl == vdr_op2_data_ptr->vdr_.vdr_fl )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr_fl != vdr_op2_data_ptr->vdr_.vdr_fl )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr_fl > vdr_op2_data_ptr->vdr_.vdr_fl )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr_fl >= vdr_op2_data_ptr->vdr_.vdr_fl )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr_fl < vdr_op2_data_ptr->vdr_.vdr_fl )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr_fl <= vdr_op2_data_ptr->vdr_.vdr_fl )
				{
					return;
				}
			}
		}

		else if( parser::is_immediate( this->m_operand2 ) )
		{
			if( this->m_datatype == "float" )
			{
				const auto imm = std::stof( this->m_operand2 );

				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr_fl == imm )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr_fl != imm )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr_fl > imm )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr_fl >= imm )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr_fl < imm )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr_fl <= imm )
				{
					return;
				}
			}

			int64_t imm = {};

			// addres is an immediate, so I convert it easily
			if (this->m_operand2.starts_with("0x"))
				imm = std::stoi(this->m_operand2, nullptr, 16);
			else
				imm = std::stoi(this->m_operand2);

			if( this->m_datatype == "qword" )
			{
				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr64s == imm )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr64s != imm )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr64s > imm )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr64s >= imm )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr64s < imm )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr64s <= imm )
				{
					return;
				}
			}

			else if( this->m_datatype == "byte" )
			{
				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr8s == static_cast< int8_t >( imm ) )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr8s != static_cast< int8_t >( imm ) )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr8s > static_cast< int8_t >( imm ) )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr8s >= static_cast< int8_t >( imm ) )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr8s < static_cast< int8_t >( imm ) )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr8s <= static_cast< int8_t >( imm ) )
				{
					return;
				}
			}

			else if( this->m_datatype == "word" )
			{
				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr16s == static_cast< int16_t >( imm ) )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr16s != static_cast< int16_t >( imm ) )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr16s > static_cast< int16_t >( imm ) )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr16s >= static_cast< int16_t >( imm ) )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr16s < static_cast< int16_t >( imm ) )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr16s <= static_cast< int16_t >( imm ) )
				{
					return;
				}
			}

			else if( this->m_datatype == "dword" )
			{
				if( this->m_op == "==" && vdr_op1_data_ptr->vdr_.vdr32s == static_cast< int32_t >( imm ) )
				{
					return;
				}

				if( this->m_op == "!=" && vdr_op1_data_ptr->vdr_.vdr32s != static_cast< int32_t >( imm ) )
				{
					return;
				}

				if( this->m_op == ">" && vdr_op1_data_ptr->vdr_.vdr32s > static_cast< int32_t >( imm ) )
				{
					return;
				}

				if( this->m_op == ">=" && vdr_op1_data_ptr->vdr_.vdr32s >= static_cast< int32_t >( imm ) )
				{
					return;
				}

				if( this->m_op == "<" && vdr_op1_data_ptr->vdr_.vdr32s < static_cast< int32_t >( imm ) )
				{
					return;
				}

				if( this->m_op == "<=" && vdr_op1_data_ptr->vdr_.vdr32s <= static_cast< int32_t >( imm ) )
				{
					return;
				}
			}
		}

		env->set_pc( env->get_pc() + this->m_insts_size + 1 );
	}

private:
	size_t      m_insts_size;
	std::string m_datatype, m_operand1, m_op, m_operand2;
};
