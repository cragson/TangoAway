#include "windows_api.hpp"
#include "logging.hpp"

#include <TlHelp32.h>

bool TangoAway_WindowsAPI::initialize_environment( const std::shared_ptr< environment >& env,
                                                   const std::string&                    process_name
)
{
	if( env == nullptr || process_name.empty() )
		return false;

	Log::info( "[TangoAway_WindowsAPI] Trying to initialize env with %s\n", process_name.c_str() );

	// first find target process by name

	const auto snapshot_handle = CreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, NULL );
	if( !snapshot_handle )
		return false;

	Log::info( "[TangoAway_WindowsAPI] Snapshot Handle: 0x%p\n", snapshot_handle );

	auto pe32   = PROCESSENTRY32();
	pe32.dwSize = sizeof( PROCESSENTRY32 );

	DWORD pid = 0;

	if( Process32First( snapshot_handle, &pe32 ) )
	{
		do
		{
			if( const auto wprocess_name = std::wstring( pe32.szExeFile ); wprocess_name == std::wstring(
				process_name.begin(), process_name.end()
			) )
			{
				pid = pe32.th32ProcessID;

				break;
			}
		}
		while( Process32Next( snapshot_handle, &pe32 ) );
	}
	else
		return false;

	Log::info( "[TangoAway_WindowsAPI] %s has pid %ld\n", process_name.c_str(), pid );

	const auto proc = OpenProcess( PROCESS_ALL_ACCESS, FALSE, pid );

	if( !proc )
		return false;

	Log::info( "[TangoAway_WindowsAPI] Process handle: 0x%p\n", proc );

	CloseHandle( snapshot_handle );

	env->set_target_process_pid( pid );
	env->set_target_process_handle( proc );

	return true;
}

std::uintptr_t TangoAway_WindowsAPI::get_image_base( const std::shared_ptr< environment >& env,
                                                     const std::string&                    image_name
)
{
	MODULEENTRY32 me32 = { sizeof( MODULEENTRY32 ) };

	const auto snapshot_handle = CreateToolhelp32Snapshot(
		TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, env->get_pid_of_target_process()
	);

	if( !snapshot_handle || snapshot_handle == INVALID_HANDLE_VALUE )
		return false;

	std::uintptr_t ret = {};

	if( Module32First( snapshot_handle, &me32 ) )
	{
		do
		{
			if( me32.szModule == std::wstring( image_name.begin(), image_name.end() ) )
			{
				ret = reinterpret_cast< std::uintptr_t >( me32.modBaseAddr );
				break;
			}
		}
		while( Module32Next( snapshot_handle, &me32 ) );
	}

	// make sure to close the handle 
	CloseHandle( snapshot_handle );

	return ret;
}

size_t TangoAway_WindowsAPI::get_image_size( const std::shared_ptr< environment >& env, const std::string& image_name )
{
	MODULEENTRY32 me32 = { sizeof( MODULEENTRY32 ) };

	const auto snapshot_handle = CreateToolhelp32Snapshot(
		TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, env->get_pid_of_target_process()
	);

	if( !snapshot_handle || snapshot_handle == INVALID_HANDLE_VALUE )
		return false;

	std::uintptr_t ret = {};

	if( Module32First( snapshot_handle, &me32 ) )
	{
		do
		{
			if( me32.szModule == std::wstring( image_name.begin(), image_name.end() ) )
			{
				ret = static_cast< size_t >( me32.modBaseSize );
				break;
			}
		}
		while( Module32Next( snapshot_handle, &me32 ) );
	}

	// make sure to close the handle 
	CloseHandle( snapshot_handle );

	return ret;
}

std::vector< uint8_t > TangoAway_WindowsAPI::read_memory_area( const std::shared_ptr< environment >& env,
                                                               const std::uintptr_t address, const size_t size
)
{
	std::vector< uint8_t > bytes;
	bytes.resize( size );

	if( !ReadProcessMemory( env->get_handle_to_target_process(), reinterpret_cast< LPCVOID >( address ), bytes.data(),
	                        size, nullptr
	) )
	{
		Log::debug( "[TangoAway_WindowsAPI::read_memory_area] Could not read the area from memory!\n" );

		return {};
	}

	return bytes;
}

std::uintptr_t TangoAway_WindowsAPI::find_signature( const std::shared_ptr< environment >& env,
                                                     const std::string& image_name, const std::string& sig
)
{
	std::vector< uint8_t > m_vecPattern;

	std::string Temp = std::string();

	std::string strPattern = sig;

	// remove all whitespace from the signature
	std::erase_if( strPattern, isspace );

	// Convert string pattern to byte pattern
	for( size_t i = 0; i < strPattern.length(); i++ )
	{
		if( strPattern.at( i ) == '?' )
		{
			m_vecPattern.emplace_back( 0xCC );
			continue;
		}

		if( Temp.length() != 2 )
			Temp += strPattern.at( i );

		if( Temp.length() == 2 )
		{
			std::erase_if( Temp, isspace );
			auto converted_pattern_byte = strtol( Temp.c_str(), nullptr, 16 ) & 0xFFF;
			m_vecPattern.emplace_back( converted_pattern_byte );
			Temp.clear();
		}
	}
	const auto vector_size = m_vecPattern.size();

	// m_vecPattern contains now the converted byte pattern
	// Search now the memory area

	const auto image_base = get_image_base( env, image_name );

	const auto image_size = get_image_size( env, image_name );

	if( !image_base || !image_size )
		return 0;

	bool           found      = false;
	std::uintptr_t found_addr = 0;

	const auto bytes = read_memory_area( env, image_base, image_size );

	if( bytes.empty() )
		return 0;

	for( std::uintptr_t current_addr = 0; current_addr < bytes.size(); current_addr++ )
	{
		if( found )
			break;

		for( uint8_t i = 0; i < vector_size; i++ )
		{
			const auto current_byte = bytes.at( current_addr + i );

			const auto pattern_byte = m_vecPattern.at( i );

			if( static_cast< uint8_t >( pattern_byte ) == 0xCC )
			{
				if( i == vector_size - 1 )
				{
					found      = true;
					found_addr = current_addr;
					break;
				}
				continue;
			}

			if( static_cast< uint8_t >( current_byte ) != pattern_byte )
				break;

			if( i == vector_size - 1 )
			{
				found_addr = current_addr;
				found      = true;
			}
		}
	}

	return image_base + found_addr;
}

bool TangoAway_WindowsAPI::write_protected_memory( const std::shared_ptr< environment >& env,
                                                   const std::uintptr_t address, const std::vector< uint8_t >& bytes
)
{
	DWORD old = 0;

	if( !VirtualProtectEx( env->get_handle_to_target_process(), reinterpret_cast< LPVOID >( address ), bytes.size(),
	                       PAGE_EXECUTE_READWRITE, &old
	) )
		return false;

	if( !WriteProcessMemory( env->get_handle_to_target_process(), reinterpret_cast< LPVOID >( address ), bytes.data(),
	                         bytes.size(), nullptr
	) )
		return false;

	if( !VirtualProtectEx( env->get_handle_to_target_process(), reinterpret_cast< LPVOID >( address ), bytes.size(),
	                       old, &old
	) )
		return false;

	return true;
}

void TangoAway_WindowsAPI::suspend_process( const std::shared_ptr< environment >& env )
{
	const auto ntdll_handle = GetModuleHandleA( "ntdll" );

	if( !ntdll_handle )
		return;

	const auto addr_ntsus = GetProcAddress( ntdll_handle, "NtSuspendProcess" );

	if( !addr_ntsus )
		return;

	using fn_NtSuspendProcess = NTSTATUS(NTAPI*)( HANDLE );

	reinterpret_cast< fn_NtSuspendProcess >( addr_ntsus )( env->get_handle_to_target_process() );

	Log::debug( "[TangoAway_WindowsAPI::suspend_process] Suspended process with pid: %lld\n",
	            env->get_pid_of_target_process()
	);
}

void TangoAway_WindowsAPI::resume_process( const std::shared_ptr< environment >& env )
{
	const auto ntdll_handle = GetModuleHandleA( "ntdll" );

	if( !ntdll_handle )
		return;

	const auto addr_ntret = GetProcAddress( ntdll_handle, "NtResumeProcess" );

	if( !addr_ntret )
		return;

	using fn_NtResumeProcess = NTSTATUS(NTAPI*)( HANDLE );

	reinterpret_cast< fn_NtResumeProcess >( addr_ntret )( env->get_handle_to_target_process() );

	Log::debug( "[TangoAway_WindowsAPI::resume_process] Resumed process with pid: %lld\n",
	            env->get_pid_of_target_process()
	);
}

bool TangoAway_WindowsAPI::inject_loadlibrary( const std::shared_ptr< environment >& env, const std::string& dll_path )
{
	const auto kernel32_handle = GetModuleHandleA( "kernel32.dll" );

	if( !kernel32_handle )
		return false;

	const auto addr_loadlibrary = GetProcAddress( kernel32_handle, "LoadLibraryA" );

	if( !addr_loadlibrary )
		return false;

	const auto allocated_mem = VirtualAllocEx( env->get_handle_to_target_process(), nullptr, dll_path.size() + 1,
	                                           MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE
	);

	if( !allocated_mem )
		return false;

	if( !WriteProcessMemory( env->get_handle_to_target_process(), allocated_mem, dll_path.c_str(), dll_path.size() + 1,
	                         nullptr
	) )
		return false;

	const auto thread_handle = CreateRemoteThread( env->get_handle_to_target_process(), nullptr, 0,
	                                               reinterpret_cast< LPTHREAD_START_ROUTINE >( addr_loadlibrary ),
	                                               allocated_mem, 0, nullptr
	);

	if( !thread_handle )
		return false;

	return true;
}

void TangoAway_WindowsAPI::print_threads_info( const std::shared_ptr< environment >& env )
{
	const auto snap = CreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, env->get_pid_of_target_process() );

	if( !snap )
	{
		Log::info( "[TangoAway_WindowsAPI::print_threads_info] Could not create snapshot handle to target pid!\n" );

		return;
	}

	THREADENTRY32 th32 = { sizeof( THREADENTRY32 ) };

	if( Thread32First( snap, &th32 ) )
	{
		do
		{
			if( !env->is_printing() )
				continue;

			printf( "[%s] OwnerPID: %lu, threadID: %lu\n", env->get_name().c_str(), th32.th32OwnerProcessID,
			        th32.th32ThreadID
			);
		}
		while( Thread32Next( snap, &th32 ) );
	}

	CloseHandle( snap );
}

void TangoAway_WindowsAPI::print_images_info( const std::shared_ptr< environment >& env )
{
	const auto snap = CreateToolhelp32Snapshot( TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32,
	                                            env->get_pid_of_target_process()
	);

	if( !snap )
	{
		Log::info( "[TangoAway_WindowsAPI::print_images_info] Could not create snapshot handle to target pid!\n" );

		return;
	}

	MODULEENTRY32 me32 = { sizeof( MODULEENTRY32 ) };

	if( Module32First( snap, &me32 ) )
	{
		do
		{
			if( !env->is_printing() )
				continue;

			printf( "[%s] name: %ws, base: 0x%p, size: 0x%lX, path: %ws\n", env->get_name().c_str(), me32.szModule,
			        me32.modBaseAddr, me32.modBaseSize, me32.szExePath
			);
		}
		while( Module32Next( snap, &me32 ) );
	}

	CloseHandle( snap );
}
