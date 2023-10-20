#pragma once
#include "instruction.hpp"
#include "utils.hpp"


class inst_xor_vdr_imm : public instruction
{
public:
	inst_xor_vdr_imm( const std::string& type, const EVdrIdentifier dest_ident, const std::string& src )
	{
		this->m_datatype    = type;
		this->m_destination = dest_ident;
		this->m_source      = src;
	}

	void evaluate(std::shared_ptr< environment > env ) override
	{
		const auto num = Utils::str_to_ul( this->m_source );

		if( m_datatype == "byte" )
		{
			auto ret = env->read_from_vdr( m_destination )->vdr_.vdr8;
			ret ^= num;

			env->write_to_vdr< uint8_t >( m_destination, ret );

			return;
		}

		if( m_datatype == "word" )
		{
			auto ret = env->read_from_vdr( m_destination )->vdr_.vdr16;
			ret ^= num;

			env->write_to_vdr< uint16_t >( m_destination, ret );

			return;
		}

		if( m_datatype == "dword" )
		{
			auto ret = env->read_from_vdr( m_destination )->vdr_.vdr32;
			ret ^= num;

			env->write_to_vdr< uint32_t >( m_destination, ret );

			return;
		}

		auto ret = env->read_from_vdr( m_destination )->vdr_.vdr64;
		ret ^= num;

		env->write_to_vdr< uint64_t >( m_destination, ret );

		return;
	}

private:
	std::string    m_datatype;
	EVdrIdentifier m_destination;
	std::string    m_source;
};
