#pragma once
#include <memory>
#include <vector>

#include "environment.hpp"

namespace TangoAway_WindowsAPI
{
	template< typename T >
	[[nodiscard]] T read_memory( const std::shared_ptr< environment >& env, const std::uintptr_t address,
	                             const size_t                          size = sizeof( T )
	)
	{
		T buffer = {};

		ReadProcessMemory( env->get_handle_to_target_process(), reinterpret_cast< LPCVOID >( address ), &buffer,
		                   sizeof( T ), nullptr
		);

		return buffer;
	}

	template< typename T >
	bool write_memory( const std::shared_ptr< environment >& env, const std::uintptr_t address, const T& value )
	{
		return WriteProcessMemory( env->get_handle_to_target_process(), reinterpret_cast< LPVOID >( address ), &value,
		                           sizeof( value ), nullptr
		) != 0;
	}

	[[nodiscard]] bool initialize_environment( const std::shared_ptr< environment >& env,
	                                           const std::string&                    process_name
	);

	[[nodiscard]] std::uintptr_t get_image_base( const std::shared_ptr< environment >& env,
	                                             const std::string&                    image_name
	);

	[[nodiscard]] size_t get_image_size( const std::shared_ptr< environment >& env, const std::string& image_name );

	[[nodiscard]] std::vector< uint8_t > read_memory_area( const std::shared_ptr< environment >& env,
	                                                       const std::uintptr_t address, const size_t size
	);

	[[nodiscard]] std::uintptr_t find_signature( const std::shared_ptr< environment >& env,
	                                             const std::string& image_name, const std::string& sig
	);

	[[nodiscard]] bool write_protected_memory( const std::shared_ptr< environment >& env, const std::uintptr_t address,
	                                           const std::vector< uint8_t >&         bytes
	);

	void suspend_process( const std::shared_ptr< environment >& env );

	void resume_process( const std::shared_ptr< environment >& env );

	[[nodiscard]] bool inject_loadlibrary( const std::shared_ptr< environment >& env, const std::string& dll_path );

	void print_threads_info( const std::shared_ptr< environment >& env );

	void print_images_info( const std::shared_ptr< environment >& env );
}
