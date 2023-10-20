#pragma once
#include "instruction.hpp"

class inst_div_vdr_vdr : public instruction
{
public:
	inst_div_vdr_vdr( const std::string& type, const EVdrIdentifier dest_ident, const EVdrIdentifier src_ident )
	{
		this->m_datatype    = type;
		this->m_destination = dest_ident;
		this->m_source      = src_ident;
	}

	void evaluate(std::shared_ptr< environment > env ) override
	{
		if( m_datatype == "byte" )
		{
			env->write_to_vdr< uint8_t >( this->m_destination,
			                              env->read_from_vdr( this->m_destination )->vdr_.vdr8 / env->read_from_vdr(
				                              this->m_source
			                              )->vdr_.vdr8
			);

			return;
		}

		if( m_datatype == "word" )
		{
			env->write_to_vdr< uint16_t >( this->m_destination,
			                               env->read_from_vdr( this->m_destination )->vdr_.vdr16 / env->read_from_vdr(
				                               this->m_source
			                               )->vdr_.vdr16
			);

			return;
		}

		if( m_datatype == "dword" )
		{
			env->write_to_vdr< uint32_t >( this->m_destination,
			                               env->read_from_vdr( this->m_destination )->vdr_.vdr32 / env->read_from_vdr(
				                               this->m_source
			                               )->vdr_.vdr32
			);

			return;
		}

		if( m_datatype == "qword" )
		{
			env->write_to_vdr< uint64_t >( this->m_destination,
			                               env->read_from_vdr( this->m_destination )->vdr_.vdr64 / env->read_from_vdr(
				                               this->m_source
			                               )->vdr_.vdr64
			);

			return;
		}

		// I can do this outside of an if-clause because I checked the input from this instance
		// so I can be sure the last case is always a float
		env->write_to_vdr< float >( this->m_destination,
		                            env->read_from_vdr( this->m_destination )->vdr_.vdr_fl / env->read_from_vdr(
			                            this->m_source
		                            )->vdr_.vdr_fl
		);
	}

private:
	std::string    m_datatype;
	EVdrIdentifier m_destination;
	EVdrIdentifier m_source;
};
