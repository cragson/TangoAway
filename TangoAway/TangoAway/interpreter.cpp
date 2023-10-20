#include "interpreter.hpp"

#include "logging.hpp"

#include "windows_api.hpp"

EStatusCode interpreter::execute_on_env( const std::string& env_name, const std::string& path_to_code_file,
                                         const std::string& target_process_name
)
{
	// check if environment name is not in the map
	// a environment is needed, so it must be in the map
	if( !this->m_envs.contains( env_name ) )
		return INTERPRETER_ENVIRONMENT_DOES_NOT_EXIST;

	// get a pointer to the used environment
	const auto env = this->m_envs[ env_name ];

	// Initialize the WindowsAPI for the env
	if( !env->is_initialized() )
	{
		if( !TangoAway_WindowsAPI::initialize_environment( env, target_process_name ) )
			return INTERPRETER_WINDOWS_API_ERROR;

		Log::info( "[interpreter::execute_on_env] initialized env %s with %s\n", env_name.c_str(),
		           target_process_name.c_str()
		);

		env->mark_as_initialized();
	}
#ifdef TANGO_AWAY_BENCHMARK_MODE
	auto t1 = std::chrono::high_resolution_clock::now();
#endif

	// preprocess the file
	const auto ret_proc = this->m_preprocessor->process_file( path_to_code_file );

	if( ret_proc != PREPROCESS_SUCCESS )
		return ret_proc;

#ifdef TANGO_AWAY_BENCHMARK_MODE
	auto t2 = std::chrono::high_resolution_clock::now();

	auto duration1 = std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 ).count();
	auto duration2 = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 ).count();

	printf( "[Benchmark-Preprocessor-Execution] %lld microseconds, %lld milliseconds\n", duration1, duration2 );
#endif

	// check if the parser has instructions in it, which may be leftover from any execution
	// if so clear them to be ready for a clean execution
	if( this->m_parser->has_instructions() )
		this->m_parser->clear_instructions();

	// make local copy of the path of the code file, which can be modified
	std::string prp_path = path_to_code_file;

	// replace now the file ending
	prp_path.replace( prp_path.find_last_of( ".ta" ) - 2, 4, ".prp" );

#ifdef TANGO_AWAY_BENCHMARK_MODE
	t1 = std::chrono::high_resolution_clock::now();
#endif

	// parse the file
	const auto ret_parser = this->m_parser->parse_file( prp_path );

#ifdef TANGO_AWAY_BENCHMARK_MODE
	t2 = std::chrono::high_resolution_clock::now();

	duration1 = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
	duration2 = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

	printf("[Benchmark-Parser-Execution] %lld microseconds, %lld milliseconds\n", duration1, duration2);
#endif

	if( ret_parser != PARSER_SUCCESS )
		return ret_parser;

	// get the size of all parsed instructions
	const auto insts_size = this->m_parser->get_instructions_size();

#ifdef TANGO_AWAY_BENCHMARK_MODE
	t1 = std::chrono::high_resolution_clock::now();
#endif

	// execute now the instructions
	for( size_t idx = env->get_pc(); idx < insts_size; idx++ )
	{
		const auto pc = env->get_pc();

		// evaluate/interprete next instruction
		this->m_parser->get_instruction_by_index( idx )->evaluate( env );

		Log::info( "[interpreter::execute_on_env] PC:0x%llX (%d/%lld) executed\n", pc + 1, idx + 1, insts_size );

		// only increase the pc, if nothing already did something to the pc
		// like jmp's
		if( pc == env->get_pc() )
		{
			// increase programm counter
			env->increase_pc();
		}
		else if( env->get_pc() == ( std::numeric_limits< size_t >::max )() )
		{
			// rpcae instruction or end of pc was reached, so exit
			// before reset pc to zero
			env->reset_pc();
			break;
		}
		else
		{
			// correct the idx and fix it by reducing one, because of the for loop
			idx = env->get_pc() - 1;
		}
	}

#ifdef TANGO_AWAY_BENCHMARK_MODE
	t2 = std::chrono::high_resolution_clock::now();

	duration1 = std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 ).count();
	duration2 = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 ).count();

	printf( "[Benchmark-Interpreter-Execution] %lld microseconds, %lld milliseconds\n", duration1, duration2 );
#endif

	return INTERPRETER_SUCCESS;
}

EStatusCode interpreter::execute_on_env( const std::string& env_name, const std::string&      path_to_code_file,
                                         const std::string& target_process_name, const size_t chunking_thread_count
)
{
#ifdef TANGO_AWAY_BENCHMARK_MODE
	printf( "running benchmark with %llu threads for chunking\n", chunking_thread_count );
#endif

	// check if environment name is not in the map
	// a environment is needed, so it must be in the map
	if( !this->m_envs.contains( env_name ) )
		return INTERPRETER_ENVIRONMENT_DOES_NOT_EXIST;

	// get a pointer to the used environment
	const auto env = this->m_envs[ env_name ];

	// Initialize the WindowsAPI for the env
	if( !env->is_initialized() )
	{
		if( !TangoAway_WindowsAPI::initialize_environment( env, target_process_name ) )
			return INTERPRETER_WINDOWS_API_ERROR;

		Log::info( "[interpreter::execute_on_env] initialized env %s with %s\n", env_name.c_str(),
		           target_process_name.c_str()
		);

		env->mark_as_initialized();
	}
#ifdef TANGO_AWAY_BENCHMARK_MODE
	long long total_duration_mi = {};
	long long total_duration_ms = {};

	auto t1 = std::chrono::high_resolution_clock::now();
#endif

	// preprocess the file
	const auto ret_proc = this->m_preprocessor->process_file( path_to_code_file );

	if( ret_proc != PREPROCESS_SUCCESS )
		return ret_proc;

#ifdef TANGO_AWAY_BENCHMARK_MODE
	auto t2 = std::chrono::high_resolution_clock::now();

	auto duration1 = std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 ).count();
	auto duration2 = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 ).count();

	total_duration_mi += duration1;
	total_duration_ms += duration2;

	printf( "[Benchmark-Preprocessor-Execution] %lld microseconds, %lld milliseconds\n", duration1, duration2 );
#endif

	// check if the parser has instructions in it, which may be leftover from any execution
	// if so clear them to be ready for a clean execution
	if( this->m_parser->has_instructions() )
		this->m_parser->clear_instructions();

	// make local copy of the path of the code file, which can be modified
	std::string prp_path = path_to_code_file;

	// replace now the file ending
	prp_path.replace( prp_path.find_last_of( ".ta" ) - 2, 4, ".prp" );

#ifdef TANGO_AWAY_BENCHMARK_MODE
	t1 = std::chrono::high_resolution_clock::now();
#endif

	// parse the file
	const auto ret_parser = this->m_parser->parse_file_multithreaded_chunking( prp_path, chunking_thread_count );

	if( ret_parser != PARSER_SUCCESS )
		return ret_parser;

#ifdef TANGO_AWAY_BENCHMARK_MODE
	t2 = std::chrono::high_resolution_clock::now();

	duration1 = std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 ).count();
	duration2 = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 ).count();

	total_duration_mi += duration1;
	total_duration_ms += duration2;

	printf( "[Benchmark-Parser-Execution] %lld microseconds, %lld milliseconds\n", duration1, duration2 );
#endif

	// get the size of all parsed instructions
	const auto insts_size = this->m_parser->get_instructions_size();

#ifdef TANGO_AWAY_BENCHMARK_MODE
	t1 = std::chrono::high_resolution_clock::now();
#endif

	// execute now the instructions
	for( size_t idx = env->get_pc(); idx < insts_size; idx++ )
	{
		const auto pc = env->get_pc();

		// evaluate/interprete next instruction
		this->m_parser->get_instruction_by_index( idx )->evaluate( env );

		Log::info( "[interpreter::execute_on_env] PC:0x%llX (%d/%lld) executed\n", pc + 1, idx + 1, insts_size );

		// only increase the pc, if nothing already did something to the pc
		// like jmp's
		if( pc == env->get_pc() )
		{
			// increase programm counter
			env->increase_pc();
		}
		else if( env->get_pc() == ( std::numeric_limits< size_t >::max )() )
		{
			// rpcae instruction or end of pc was reached, so exit
			// before reset pc to zero
			env->reset_pc();
			break;
		}
		else
		{
			// correct the idx and fix it by reducing one, because of the for loop
			idx = env->get_pc() - 1;
		}
	}

#ifdef TANGO_AWAY_BENCHMARK_MODE
	t2 = std::chrono::high_resolution_clock::now();

	duration1 = std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 ).count();
	duration2 = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 ).count();

	total_duration_mi += duration1;
	total_duration_ms += duration2;

	printf( "[Benchmark-Interpreter-Execution] %lld microseconds, %lld milliseconds\n", duration1, duration2 );

	printf( "[Benchmark-Total-Execution] %lld microseconds, %lld milliseconds\n", total_duration_mi, total_duration_ms
	);
#endif

	return INTERPRETER_SUCCESS;
}

EStatusCode interpreter::execute_on_env( const std::string& env_name, const std::string&    path_to_code_file,
                                         const std::string& target_process_name, const bool auto_chunking
)
{
	return this->execute_on_env( env_name, path_to_code_file, target_process_name,
	                             auto_chunking ? std::thread::hardware_concurrency() : size_t() + 1
	);
}

EStatusCode interpreter::execute_on_env( const std::string&                                   env_name,
                                         const std::vector< std::shared_ptr< instruction > >& insts,
                                         const std::string&                                   target_process_name
)
{
	// check if environment name is not in the map
	// a environment is needed, so it must be in the map
	if( !this->m_envs.contains( env_name ) )
		return INTERPRETER_ENVIRONMENT_DOES_NOT_EXIST;

	// get a pointer to the used environment
	const auto env = this->m_envs[ env_name ];

	// Initialize the WindowsAPI for the env
	if( !env->is_initialized() )
	{
		if( !TangoAway_WindowsAPI::initialize_environment( env, target_process_name ) )
			return INTERPRETER_WINDOWS_API_ERROR;

		Log::info( "[interpreter::execute_on_env] initialized env %s with %s\n", env_name.c_str(),
		           target_process_name.c_str()
		);

		env->mark_as_initialized();
	}

	// get the size of all parsed instructions
	const auto insts_size = insts.size();

	// execute now the instructions
	for( size_t idx = env->get_pc(); idx < insts_size; idx++ )
	{
		const auto pc = env->get_pc();

		// evaluate/interprete next instruction
		insts.at( idx )->evaluate(env);

		Log::info( "[interpreter::execute_on_env] PC:0x%llX (%d/%lld) executed\n", pc + 1, idx + 1, insts_size );

		// only increase the pc, if nothing already did something to the pc
		// like jmp's
		if( pc == env->get_pc() )
		{
			// increase programm counter
			env->increase_pc();
		}
		else if( env->get_pc() == ( std::numeric_limits< size_t >::max )() )
		{
			// rpcae instruction or end of pc was reached, so exit
			// before reset pc to zero
			env->reset_pc();
			break;
		}
		else
		{
			// correct the idx and fix it by reducing one, because of the for loop
			idx = env->get_pc() - 1;
		}
	}

	return INTERPRETER_SUCCESS;
}
