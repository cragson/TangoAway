#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "environment.hpp"
#include "parser.hpp"
#include "preprocessor.hpp"

// Enable Benchmarking
//#define TANGO_AWAY_BENCHMARK_MODE

class interpreter
{
public:
	interpreter()
	{
		this->m_envs = {};

		this->m_preprocessor = std::make_unique< preprocessor >();

		this->m_parser = std::make_unique< parser >();
	}

	[[nodiscard]] EStatusCode load_env( const std::string& name, const std::shared_ptr< environment >& env )
	{
		if( !env )
			return INTERPRETER_INVALID_ENVIRONMENT_POINTER;

		if( name.empty() || this->m_envs.contains( name ) )
			return INTERPRETER_INVALID_ENVIRONMENT_NAME;

		this->m_envs[ name ] = env;

		return INTERPRETER_SUCCESS;
	}

	[[nodiscard]] EStatusCode create_env( const std::string& name )
	{
		if( name.empty() || this->m_envs.contains( name ) )
			return INTERPRETER_INVALID_ENVIRONMENT_NAME;

		this->m_envs[ name ] = std::make_shared< environment >();

		this->m_envs[ name ]->set_name( name );

		return INTERPRETER_SUCCESS;
	}

	inline void print_vdr_from_env( const std::string& name ) noexcept
	{
		if( name.empty() || !this->m_envs.contains( name ) )
			return;

		this->m_envs[ name ]->print_vdr();
	}

	inline void print_vfr_from_env( const std::string& name ) noexcept
	{
		if( name.empty() || !this->m_envs.contains( name ) )
			return;

		this->m_envs[ name ]->print_vfr();
	}

	inline void clear_envs()
	{
		this->m_envs.clear();
	}

	[[nodiscard]] inline bool reset_pc_from_env( const std::string& name )
	{
		if( !this->m_envs.contains( name ) )
			return false;

		this->m_envs[ name ]->reset_pc();

		return true;
	}

	[[nodiscard]] inline bool reset_env( const std::string& name )
	{
		if( !this->m_envs.contains( name ) )
			return false;

		this->m_envs[ name ] = std::make_shared< environment >( name );

		return true;
	}

	[[nodiscard]] inline bool does_env_exists( const std::string& name ) const noexcept
	{
		return this->m_envs.contains( name );
	}

	[[nodiscard]] inline size_t get_size_of_envs() const noexcept
	{
		return this->m_envs.size();
	}

	[[nodiscard]] EStatusCode execute_on_env( const std::string& env_name, const std::string& path_to_code_file,
	                                          const std::string& target_process_name
	);

	[[nodiscard]] EStatusCode execute_on_env( const std::string& env_name, const std::string&      path_to_code_file,
	                                          const std::string& target_process_name, const size_t chunking_thread_count
	);

	[[nodiscard]] EStatusCode execute_on_env( const std::string& env_name, const std::string&    path_to_code_file,
	                                          const std::string& target_process_name, const bool auto_chunking
	);

	[[nodiscard]] EStatusCode execute_on_env( const std::string&                                   env_name,
	                                          const std::vector< std::shared_ptr< instruction > >& insts,
	                                          const std::string&                                   target_process_name
	);

	[[nodiscard]] inline auto get_preprocessor() const noexcept
	{
		return &this->m_preprocessor;
	}

	[[nodiscard]] inline auto get_parser() const noexcept
	{
		return &this->m_parser;
	}

	[[nodiscard]] inline auto get_vdr_from_env( const EVdrIdentifier ident, const std::string& env_name )
	{
		return this->m_envs[ env_name ]->read_from_vdr( ident )->vdr_;
	}

	[[nodiscard]] inline auto get_vfr_from_env( const std::string& env_name )
	{
		return this->m_envs[ env_name ]->get_vfr();
	}

	inline void enable_printing_from_env( const std::string& name )
	{
		this->m_envs[ name ]->enable_printing();
	}

	inline void disable_printing_from_env( const std::string& name )
	{
		this->m_envs[ name ]->disable_printing();
	}

	inline void toggle_printing_from_env( const std::string& name )
	{
		this->m_envs[ name ]->toggle_printing();
	}

	[[nodiscard]] inline auto is_printing_from_env( const std::string& name )
	{
		return this->m_envs[ name ]->is_printing();
	}

private:
	std::unordered_map< std::string, std::shared_ptr< environment > > m_envs;

	std::unique_ptr< preprocessor > m_preprocessor;

	std::unique_ptr< parser > m_parser;
};
