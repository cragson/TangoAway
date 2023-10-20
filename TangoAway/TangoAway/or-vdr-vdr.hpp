#pragma once
#include "instruction.hpp"

class inst_or_vdr_vdr : public instruction
{
public:
	inst_or_vdr_vdr( const std::string& type, const EVdrIdentifier dest_ident, const EVdrIdentifier src_ident )
	{
		this->m_datatype    = type;
		this->m_destination = dest_ident;
		this->m_source      = src_ident;
	}

	void evaluate(std::shared_ptr< environment > env ) override
	{
		if( m_datatype == "byte" )
		{
			auto ret = env->read_from_vdr( m_destination )->vdr_.vdr8;
			ret |= env->read_from_vdr( m_source )->vdr_.vdr8;

			env->write_to_vdr< uint8_t >( m_destination, ret );

			return;
		}

		if( m_datatype == "word" )
		{
			auto ret = env->read_from_vdr( m_destination )->vdr_.vdr16;
			ret |= env->read_from_vdr( m_source )->vdr_.vdr16;

			env->write_to_vdr< uint16_t >( m_destination, ret );

			return;
		}

		if( m_datatype == "dword" )
		{
			auto ret = env->read_from_vdr( m_destination )->vdr_.vdr32;
			ret |= env->read_from_vdr( m_source )->vdr_.vdr32;

			env->write_to_vdr< uint32_t >( m_destination, ret );

			return;
		}

		if( m_datatype == "qword" )
		{
			auto ret = env->read_from_vdr( m_destination )->vdr_.vdr64;
			ret |= env->read_from_vdr( m_source )->vdr_.vdr64;

			env->write_to_vdr< uint64_t >( m_destination, ret );
		}
	}

private:
	std::string    m_datatype;
	EVdrIdentifier m_destination;
	EVdrIdentifier m_source;
};
