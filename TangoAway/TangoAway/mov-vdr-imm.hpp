#pragma once
#include "instruction.hpp"
#include "utils.hpp"


class inst_mov_vdr_imm : public instruction
{
public:
	inst_mov_vdr_imm( const std::string& type, const EVdrIdentifier dest_ident, const std::string& src )
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
			env->write_to_vdr< uint8_t >( this->m_destination, num );

			return;
		}

		if( m_datatype == "word" )
		{
			env->write_to_vdr< uint16_t >( this->m_destination, num );

			return;
		}

		if( m_datatype == "dword" )
		{
			env->write_to_vdr< uint32_t >( this->m_destination, num );

			return;
		}

		if( m_datatype == "qword" )
		{
			env->write_to_vdr< uint64_t >( this->m_destination, num );

			return;
		}

		// I can do this outside of an if-clause because I checked the input from this instance
		// so I can be sure the last case is always a float

		const auto fl = std::stof( this->m_source, nullptr );

		env->write_to_vdr< float >( this->m_destination, fl );

		return;
	}

private:
	std::string    m_datatype;
	EVdrIdentifier m_destination;
	std::string    m_source;
};
