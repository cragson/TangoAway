#include "test_case.hpp"

#include "logging.hpp"

void test_case::check_preprocessing()
{
	if( const auto ret = this->m_interp->get_preprocessor()->get()->process_file( this->m_path_to_ta ); ret ==
		PREPROCESS_SUCCESS )
	{
		Log::test( "[PREPROCESSOR] [SUCCESS] %s\n", this->m_path_to_ta.c_str() );

		this->m_status = TEST_SUCCESS;
	}
	else
	{
		Log::test( "[PREPROCESSOR] [FAILED] %s with %s\n", this->m_path_to_ta.c_str(), code_to_str( ret ).c_str() );

		this->m_status = TEST_FAILED;
	}
}

void test_case::check_parsing()
{
	if( this->m_status != TEST_SUCCESS )
		return;

	if( const auto ret = this->m_interp->get_parser()->get()->parse_file( m_path_to_prp ); ret == PARSER_SUCCESS )
	{
		Log::test( "[PARSER] [SUCCESS] %s\n", m_path_to_prp.c_str() );

		this->m_status = TEST_SUCCESS;
	}
	else
	{
		Log::test( "[PARSER] [FAILED] %s with %s\n", m_path_to_prp.c_str(), code_to_str( ret ).c_str() );

		this->m_status = TEST_FAILED;
	}
}

void test_case::test()
{
	this->on_case();

	if( this->m_status != TEST_SUCCESS )
	{
		Log::test( "[%-30s] FAILED\n", this->m_path_to_ta.c_str() );

		return;
	}

	Log::test( "[%-25s] SUCCESS\n", this->m_path_to_ta.c_str() );
}
