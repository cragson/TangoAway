#pragma once
#include "logging.hpp"
#include "test_case.hpp"

class tc_invalid_inc_vdr : public test_case
{
public:
	tc_invalid_inc_vdr( const std::string& path ) : test_case( path ) { }

	void on_case() override
	{
		if( const auto ret = this->m_interp->execute_on_env( this->get_test_environment_name(),
		                                                     this->get_path_to_ta_file(), this->m_target_process_name
		); ret == PARSER_INVALID_INSTRUCTION_VDR )
		{
			this->m_status = TEST_SUCCESS;

			return;
		}
		else
			Log::test( "[%s] Interpreter failed with: %s\n", this->m_path_to_ta.c_str(), code_to_str( ret ).c_str() );

		this->m_status = TEST_FAILED;
	}
};
