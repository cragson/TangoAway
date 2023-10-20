#pragma once
#include <memory>

#include "interpreter.hpp"
#include "status_code.hpp"
#include "utils.hpp"

class test_case
{
public:
	test_case() = delete;

	explicit test_case( const std::string& ta_path )
	{
		this->m_path_to_ta = ta_path;

		this->m_target_process_name = "dummy.exe";
		this->m_test_env_name = "test-env";
		this->m_path_to_prp = std::string( m_path_to_ta.substr( 0, m_path_to_ta.find_last_of( '.' ) ) + ".prp" );
		this->m_status = TEST_UNUSED;
		this->m_interp = std::make_unique< interpreter >();

		if( !this->m_interp->create_env( this->m_test_env_name ) )
			throw std::runtime_error( "Could not create test case environment!" );

		this->m_interp->disable_printing_from_env( this->m_test_env_name );
	}

	~test_case()
	{
		if( Utils::does_file_exists( this->m_path_to_prp ) )
			if( !Utils::delete_file( this->m_path_to_prp ) )
				throw std::runtime_error( "Could not delete .prp file from disk as cleanup action!" );
	}

	virtual void on_case() = 0;

	void check_preprocessing();

	void check_parsing();

	void test();

	[[nodiscard]] inline auto get_path_to_ta_file() const noexcept
	{
		return this->m_path_to_ta;
	}

	[[nodiscard]] inline auto get_path_to_prp_file() const noexcept
	{
		return this->m_path_to_prp;
	}

	[[nodiscard]] inline auto get_status() const noexcept
	{
		return this->m_status;
	}

	[[nodiscard]] inline auto get_test_environment_name() const noexcept
	{
		return this->m_test_env_name;
	}

	[[nodiscard]] inline auto get_target_process_name() const noexcept
	{
		return this->m_target_process_name;
	}

	inline void set_test_environment_name( const std::string& name )
	{
		this->m_test_env_name = name;
	}

	inline void set_target_process_name( const std::string& name )
	{
		this->m_target_process_name = name;
	}

	[[nodiscard]] inline auto get_vdr( const EVdrIdentifier ident ) const
	{
		return this->m_interp->get_vdr_from_env( ident, this->m_test_env_name );
	}

	[[nodiscard]] inline auto get_vfr() const
	{
		return this->m_interp->get_vfr_from_env( this->m_test_env_name );
	}

protected:
	std::string m_target_process_name;

	std::string m_test_env_name;

	std::string m_path_to_ta;

	std::string m_path_to_prp;

	EStatusCode m_status;

	std::unique_ptr< interpreter > m_interp;
};
