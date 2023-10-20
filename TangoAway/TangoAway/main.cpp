#include <Windows.h>
#include <TlHelp32.h>

#include "interpreter.hpp"
#include "tester.hpp"
#include "utils.hpp"

/* RE-Tool */
int re_tool()
{
	SetConsoleTitleA( "TangoAway: RE Tool" );

	const auto interp = std::make_unique< interpreter >();

	std::string current_env_name = { "re-env" };

	if( !interp->create_env( current_env_name ) )
	{
		printf( "[!] Could not create the default environment!\n" );

		return EXIT_FAILURE;
	}

	std::string current_process_name = {};

	bool is_attached = false;

	bool is_running = true;

	printf( "[!] Default environment: re-env\n" );
	printf( "[!] Use help to show available commands!\n\n" );

	const auto handle_cmd = [&]( const std::vector< std::string >& splitcmd )
	{
		if( splitcmd.empty() )
			return;

		const auto& mnemonic = splitcmd.front();

		if( mnemonic == "help" )
		{
			printf( "[+] attach -- attach <process_name> -- attach dummy.exe\n" );
			printf( "[+] bpatch -- bpatch <address> <bytes> -- bpatch 0x1337 90 90 90\n" );
			printf( "[+] change_env -- change_env <name> -- change_env re-env\n" );
			printf( "[+] dump_image -- dump_image <image_name> <dump_name> -- dump_image dummy.exe dummy.exe.dmp\n" );
			printf( "[+] dump_mem -- dump_mem <address> <size> <dump_name> -- dump_mem 0x13376077 256 nice.dmp\n" );
			printf( "[+] dump_vdr -- dump_vdr -- dump_vdr\n" );
			printf( "[+] dump_vfr -- dump_vfr -- dump_vfr\n" );
			printf( "[+] find_sig -- find_sig <signature> -- find_sig 74 05 ? ? 89 05\n" );
			printf( "[+] fnv -- fnv <address> <size> -- fnv 0xDEADAFFE 16\n" );
			printf( "[+] get_image_info -- get_image_info <image_name> -- get_image_info dummy.exe\n" );
			printf( "[+] hexdump -- hexdump <address> <size> -- hexdump 0x1337 10\n" );
			printf( "[+] help -- help -- help\n" );
			printf( "[+] inject -- inject <dll_path> <injection_method> -- inject sample.dll loadlibrary\n" );
			printf( "[+] kill -- kill -- kill\n" );
			printf( "[+] nop_fn -- nop_fn <function_address> -- nop_fn 0xCAFEAFFE\n" );
			printf( "[+] read -- read <address> -- read 0xCAFECAFE\n" );
			printf( "[+] reset -- reset -- reset\n" );
			printf( "[+] resume -- resume -- resume\n" );
			printf( "[+] set_bp -- set_bp <address> -- set_bp 0xDEADAFFE1337\n" );
			printf( "[+] show_images -- show_images -- show_images\n" );
			printf( "[+] show_threads -- show_threads -- show_threads\n" );
			printf( "[+] suspend -- suspend -- suspend\n" );
			printf( "[+] run -- run <path_to_file> -- run read_mem.ta\n" );
			printf( "[+] quit -- quit -- quit\n" );

			return;
		}

		// syntax: attach <process_name>
		if( mnemonic == "attach" )
		{
			if( splitcmd.size() != 2 )
			{
				printf( "[!] Invalid attach syntax.\n" );

				return;
			}

			current_process_name = splitcmd.back();

			printf( "[+] Attached to %s\n", current_process_name.c_str() );

			is_attached = true;

			return;
		}

		// syntax: bpatch <address> <bytes>
		if( mnemonic == "bpatch" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() < 3 )
			{
				printf( "[!] Invalid bpatch syntax.\n" );

				return;
			}

			const auto replace_marker = [&splitcmd]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@PATCHADDR@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@PATCHADDR\\@" ), splitcmd.at( 1 ) );
						}

						else if( line.contains( "@PATCHBYTES@" ) )
						{
							std::string bytes = {};

							std::ranges::for_each( splitcmd, [&splitcmd, &bytes]( const auto& elem )
							                       {
								                       if( !( elem == splitcmd.at( 0 ) || elem == splitcmd.at( 1 ) ) )
									                       bytes += elem;
							                       }
							);

							line = std::regex_replace( line, std::regex( "\\@PATCHBYTES\\@" ), bytes );
						}
					}
				}
			};

			if( !Utils::edit_file( "bpatch.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usedbpatch.ta", current_process_name ); ret
				== INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran bpatch!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usedbpatch.ta" ) )
				printf( "[!] Could not delete file: usedbpatch.ta\n" );

			if( !Utils::delete_file( "usedbpatch.prp" ) )
				printf( "[!] Could not delete file: usedbpatch.prp\n" );

			return;
		}

		// syntax: change_env <name>
		if( mnemonic == "change_env" )
		{
			if( splitcmd.size() != 2 )
			{
				printf( "[!] Invalid change_env syntax.\n" );

				return;
			}

			current_env_name = splitcmd.back();

			if( !interp->does_env_exists( current_env_name ) )
			{
				if( const auto ret = interp->create_env( current_env_name ); ret != INTERPRETER_SUCCESS )
				{
					printf( "[!] Could not create env with name %s, failed with %s\n", current_env_name.c_str(),
					        code_to_str( ret ).c_str()
					);

					is_running = false;

					return;
				}

				printf( "[+] Created env %s sucessfully!\n", current_env_name.c_str() );
			}

			printf( "[+] Changed env to %s!\n", current_env_name.c_str() );

			return;
		}

		// syntax: dump_image <image_name> <dump_name>
		if( mnemonic == "dump_image" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 3 )
			{
				printf( "[!] Invalid dump_image syntax.\n" );

				return;
			}

			const auto image_name = splitcmd.at( 1 );

			const auto dump_name = splitcmd.back();

			const auto replace_marker = [&splitcmd, &image_name, &dump_name]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@DUMPNAME@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@DUMPNAME\\@" ), image_name );
						}

						else if( line.contains( "@DUMPOUTPUTNAME@" ) )
						{
							std::string bytes = {};

							std::ranges::for_each( splitcmd, [&splitcmd, &bytes]( const auto& elem )
							                       {
								                       if( !( elem == splitcmd.at( 0 ) ) )
									                       bytes += elem;
							                       }
							);

							line = std::regex_replace( line, std::regex( "\\@DUMPOUTPUTNAME\\@" ), dump_name );
						}
					}
				}
			};

			if( !Utils::edit_file( "dump_image.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "useddump_image.ta", current_process_name );
				ret == INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran dump_image!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "useddump_image.ta" ) )
				printf( "[!] Could not delete file: useddump_image.ta\n" );

			if( !Utils::delete_file( "useddump_image.prp" ) )
				printf( "[!] Could not delete file: useddump_image.prp\n" );

			return;
		}

		// syntax: dump_mem <address> <size> <dump_name>
		if( mnemonic == "dump_mem" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 4 )
			{
				printf( "[!] Invalid dump_image syntax.\n" );

				return;
			}

			const auto dump_addr = splitcmd.at( 1 );

			const auto dump_size = splitcmd.at( 2 );

			const auto dump_name = splitcmd.back();

			const auto replace_marker = [&splitcmd, &dump_addr, &dump_size, &dump_name]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@DUMPADDR@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@DUMPADDR\\@" ), dump_addr );
						}

						else if( line.contains( "@DUMPSIZE@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@DUMPSIZE\\@" ), dump_size );
						}

						else if( line.contains( "@DUMPNAME@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@DUMPNAME\\@" ), dump_name );
						}
					}
				}
			};

			if( !Utils::edit_file( "dump_mem.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "useddump_mem.ta", current_process_name );
				ret == INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran dump_mem!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "useddump_mem.ta" ) )
				printf( "[!] Could not delete file: useddump_mem.ta\n" );

			if( !Utils::delete_file( "useddump_mem.prp" ) )
				printf( "[!] Could not delete file: useddump_mem.prp\n" );

			return;
		}

		// syntax: dump_vdr
		if( mnemonic == "dump_vdr" )
		{
			interp->print_vdr_from_env( current_env_name );

			return;
		}

		// syntax: dump_vdr
		if( mnemonic == "dump_vfr" )
		{
			interp->print_vfr_from_env( current_env_name );

			return;
		}

		// syntax: find_sig <signature>
		if( mnemonic == "find_sig" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() < 2 )
			{
				printf( "[!] Invalid find_sig syntax.\n" );

				return;
			}

			const auto replace_marker = [&splitcmd, &current_process_name]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@PROCESSNAME@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@PROCESSNAME\\@" ), current_process_name );
						}

						else if( line.contains( "@SIG@" ) )
						{
							std::string bytes = {};

							std::ranges::for_each( splitcmd, [&splitcmd, &bytes]( const auto& elem )
							                       {
								                       if( !( elem == splitcmd.at( 0 ) ) )
									                       bytes += elem;
							                       }
							);

							line = std::regex_replace( line, std::regex( "\\@SIG\\@" ), bytes );
						}
					}
				}
			};

			if( !Utils::edit_file( "find_sig.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usedfind_sig.ta", current_process_name );
				ret == INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran find_sig!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usedfind_sig.ta" ) )
				printf( "[!] Could not delete file: usedfind_sig.ta\n" );

			if( !Utils::delete_file( "usedfind_sig.prp" ) )
				printf( "[!] Could not delete file: usedfind_sig.prp\n" );

			return;
		}

		// syntax: fnv <address> <size>
		if( mnemonic == "fnv" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 3 )
			{
				printf( "[!] Invalid fnv syntax.\n" );

				return;
			}

			const auto replace_marker = [&splitcmd, &current_process_name]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@STARTADDR@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@STARTADDR\\@" ), splitcmd.at( 1 ) );
						}

						else if( line.contains( "@SIZE@" ) )
						{
							std::string bytes = {};

							std::ranges::for_each( splitcmd, [&splitcmd, &bytes]( const auto& elem )
							                       {
								                       if( !( elem == splitcmd.at( 0 ) ) )
									                       bytes += elem;
							                       }
							);

							line = std::regex_replace( line, std::regex( "\\@SIZE\\@" ), splitcmd.back() );
						}
					}
				}
			};

			if( !Utils::edit_file( "fnv.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usedfnv.ta", current_process_name ); ret ==
				INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran fnv!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usedfnv.ta" ) )
				printf( "[!] Could not delete file: usedfnv.ta\n" );

			if( !Utils::delete_file( "usedfnv.prp" ) )
				printf( "[!] Could not delete file: usedfnv.prp\n" );

			return;
		}

		// syntax: get_image_info <image_name>
		if( mnemonic == "get_image_info" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 2 )
			{
				printf( "[!] Invalid get_image_info syntax.\n" );

				return;
			}

			const auto replace_marker = [&splitcmd]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@IMAGENAME@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@IMAGENAME\\@" ), splitcmd.back() );
						}
					}
				}
			};

			if( !Utils::edit_file( "get_image_info.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usedget_image_info.ta", current_process_name
			); ret == INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran get_image_info!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usedget_image_info.ta" ) )
				printf( "[!] Could not delete file: usedget_image_info.ta\n" );

			if( !Utils::delete_file( "usedget_image_info.prp" ) )
				printf( "[!] Could not delete file: usedget_image_info.prp\n" );

			return;
		}

		// syntax: hexdump <address> <size>
		if( mnemonic == "hexdump" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 3 )
			{
				printf( "[!] Invalid hexdump syntax.\n" );

				return;
			}

			const auto replace_marker = [&splitcmd]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@DUMPADDR@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@DUMPADDR\\@" ), splitcmd.at( 1 ) );
						}

						else if( line.contains( "@DUMPSIZE@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@DUMPSIZE\\@" ), splitcmd.at( 2 ) );
						}
					}
				}
			};

			if( !Utils::edit_file( "hexdump.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usedhexdump.ta", current_process_name ); ret
				== INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran hexdump!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usedhexdump.ta" ) )
				printf( "[!] Could not delete file: usedhexdump.ta\n" );

			if( !Utils::delete_file( "usedhexdump.prp" ) )
				printf( "[!] Could not delete file: usedhexdump.prp\n" );

			return;
		}

		// syntax: kill
		if( mnemonic == "kill" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 1 )
			{
				printf( "[!] Invalid kill syntax.\n" );

				return;
			}

			const auto pid = GetCurrentProcessId();

			if( !pid )
			{
				printf( "[!] Could not retrieve pid for %s\n", current_process_name.c_str() );

				return;
			}

			const auto replace_marker = [&pid]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@PROCPID@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@PROCPID\\@" ),
							                           std::vformat( "{:d}", std::make_format_args( pid ) )
							);
						}
					}
				}
			};

			if( !Utils::edit_file( "kill.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usedkill.ta", current_process_name ); ret ==
				INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran kill!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usedkill.ta" ) )
				printf( "[!] Could not delete file: usedhexdump.ta\n" );

			if( !Utils::delete_file( "usedkill.prp" ) )
				printf( "[!] Could not delete file: usedhexdump.prp\n" );

			is_attached = false;

			is_running = false;

			return;
		}

		// syntax: inject <dll_path> <injection_method>
		if( mnemonic == "inject" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 3 )
			{
				printf( "[!] Invalid inject syntax.\n" );

				return;
			}

			const auto replace_marker = [&splitcmd]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@DLLPATH@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@DLLPATH\\@" ), splitcmd.at( 1 ) );
						}

						if( line.contains( "@INJMTH@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@INJMTH\\@" ), splitcmd.back() );
						}
					}
				}
			};

			if( !Utils::edit_file( "inject.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usedinject.ta", current_process_name ); ret
				== INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran inject!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usedinject.ta" ) )
				printf( "[!] Could not delete file: usedinject.ta\n" );

			if( !Utils::delete_file( "usedinject.prp" ) )
				printf( "[!] Could not delete file: usedinject.prp\n" );

			return;
		}

		// syntax: nop_fn <function_address>
		if( mnemonic == "nop_fn" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 2 )
			{
				printf( "[!] Invalid nop_fn syntax.\n" );

				return;
			}

			const auto replace_marker = [&splitcmd]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@FNADDR@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@FNADDR\\@" ), splitcmd.back() );
						}
					}
				}
			};

			if( !Utils::edit_file( "nop_fn.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usednop_fn.ta", current_process_name ); ret
				== INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran nop_fn!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usednop_fn.ta" ) )
				printf( "[!] Could not delete file: usednop_fn.ta\n" );

			if( !Utils::delete_file( "usednop_fn.prp" ) )
				printf( "[!] Could not delete file: usednop_fn.prp\n" );

			return;
		}

		// syntax: read <address>
		if( mnemonic == "read" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 2 )
			{
				printf( "[!] Invalid read syntax.\n" );

				return;
			}

			const auto replace_marker = [&splitcmd]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@READADDR@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@READADDR\\@" ), splitcmd.at( 1 ) );
						}
					}
				}
			};

			if( !Utils::edit_file( "read.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usedread.ta", current_process_name ); ret ==
				INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran read!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usedread.ta" ) )
				printf( "[!] Could not delete file: usedread.ta\n" );

			if( !Utils::delete_file( "usedread.prp" ) )
				printf( "[!] Could not delete file: usedread.prp\n" );

			return;
		}

		// syntax: reset
		if( mnemonic == "reset" )
		{
			if( interp->reset_env( current_env_name ) )
				printf( "\n[+] Resetted PC and VDR's on %s\n", current_env_name.c_str() );
			else
				printf( "[!] Could not reset the pc from env %s!\n", current_env_name.c_str() );

			return;
		}

		// syntax: resume
		if( mnemonic == "resume" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 1 )
			{
				printf( "[!] Invalid suspend syntax.\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "resume.ta", current_process_name ); ret ==
				INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran resume!\n" );

				return;
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			return;
		}

		// syntax: run <path_to_file>
		if( mnemonic == "run" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 2 )
			{
				printf( "[!] Invalid run syntax.\n" );

				return;
			}

			const auto& path_to_file = splitcmd.back();

			if( const auto ret = interp->execute_on_env( current_env_name, path_to_file, current_process_name ); ret !=
				INTERPRETER_SUCCESS )
			{
				printf( "[!] Could not execute %s on env %s with process %s, failed with %s\n",
				        current_env_name.c_str(), path_to_file.c_str(), current_process_name.c_str(),
				        code_to_str( ret ).c_str()
				);

				return;
			}

			return;
		}

		// syntax: set_bp <address>
		if( mnemonic == "set_bp" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 2 )
			{
				printf( "[!] Invalid set_bp syntax.\n" );

				return;
			}

			const auto replace_marker = [&splitcmd]( std::string& line )
			{
				if( !line.empty() )
				{
					const auto split_line = Utils::split_string( line );

					if( split_line.front() == "define" )
					{
						if( line.contains( "@PATCHADDR@" ) )
						{
							line = std::regex_replace( line, std::regex( "\\@PATCHADDR\\@" ), splitcmd.back() );
						}
					}
				}
			};

			if( !Utils::edit_file( "set_bp.ta", replace_marker, "used" ) )
			{
				printf( "[!] Could not prepare the template!\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "usedset_bp.ta", current_process_name ); ret
				== INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran set_bp!\n" );
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			if( !Utils::delete_file( "usedset_bp.ta" ) )
				printf( "[!] Could not delete file: usedset_bp.ta\n" );

			if( !Utils::delete_file( "usedset_bp.prp" ) )
				printf( "[!] Could not delete file: usedset_bp.prp\n" );

			return;
		}

		// syntax: show_images
		if( mnemonic == "show_images" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 1 )
			{
				printf( "[!] Invalid suspend syntax.\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "show_images.ta", current_process_name ); ret
				== INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran show_images!\n" );

				return;
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			return;
		}

		// syntax: show_threads
		if( mnemonic == "show_threads" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 1 )
			{
				printf( "[!] Invalid suspend syntax.\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "show_threads.ta", current_process_name );
				ret == INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran show_threads!\n" );

				return;
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			return;
		}

		// syntax: suspend
		if( mnemonic == "suspend" )
		{
			if( !is_attached )
			{
				printf( "[!] Can't run .ta file, if not attached to a process!\n" );

				return;
			}

			if( splitcmd.size() != 1 )
			{
				printf( "[!] Invalid suspend syntax.\n" );

				return;
			}

			if( const auto ret = interp->execute_on_env( current_env_name, "suspend.ta", current_process_name ); ret ==
				INTERPRETER_SUCCESS )
			{
				printf( "[+] Successfully ran suspend!\n" );

				return;
			}
			else
				printf( "[!] Failed with %s\n", code_to_str( ret ).c_str() );

			return;
		}

		// syntax: quit
		if( mnemonic == "quit" )
		{
			is_running = false;

			return;
		}

		printf( "[!] %s is not valid command!\n", mnemonic.c_str() );
	};

	while( is_running )
	{
		const auto user_cmd = Utils::get_user_input( "\n> " );

		const auto split_cmd = Utils::split_string( user_cmd );

		handle_cmd( split_cmd );
	}
}

/* Tester */
int test()
{
	const auto test = std::make_unique< tester >();

	if( !test->run_tests() )
	{
		printf( "[!] Failed to run tests!\n" );

		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}

int benchmarking()
{
	const auto prp        = std::make_unique< preprocessor >();
	const auto prs_single = std::make_unique< parser >();
	const auto prs_multi  = std::make_unique< parser >();

	const auto interp = std::make_unique< interpreter >();
	if( interp->create_env( "bench" ) != INTERPRETER_SUCCESS )
	{
		printf( "[!] Could not create benchmark environment for interpreter!\n" );

		return 1;
	}

	std::vector< std::string > results_csv;
	results_csv.emplace_back(
		"Index,Name,Reps,PreprocessorAvgMicros,ParserSingleAvgMicros,ParserMultiAvgMicros,InterpreterAvgMicros"
	);

	std::unordered_map< std::string, std::tuple< long long, long long, long long, long long > > results;

	constexpr auto REPS = 5;

	for( auto& dir_it : std::filesystem::directory_iterator( "..//Benchmarks" ) )
	{
		if( dir_it.is_directory() || dir_it.path().extension() != ".ta" )
			continue;

		const auto bench_file = dir_it.path().generic_string();

		printf( "[+] Benchmarking: %s\n",
		        bench_file.substr( bench_file.find( "sample_bench_" ), bench_file.find( ".ta" ) - 3 ).c_str()
		);

		for( size_t idx = 0; idx < REPS; idx++ )
		{
			auto t1 = std::chrono::high_resolution_clock::now();
			if( const auto ret = prp->process_file( bench_file ); ret != PREPROCESS_SUCCESS )
			{
				printf( "preprocesssor failed with: %s\n", code_to_str( ret ).c_str() );

				return 1;
			}
			auto t2 = std::chrono::high_resolution_clock::now();

			auto duration1 = std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 ).count();
			//auto duration2 = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 ).count();
			//printf( "[Benchmark-Preprocessor] %lld microseconds, %lld milliseconds\n", duration1, duration2 );

			std::get< 0 >( results[ bench_file ] ) = std::get< 0 >( results[ bench_file ] ) + duration1;

			const auto bench_file_prp = std::string( bench_file.substr( 0, bench_file.find_last_of( '.' ) ) + ".prp" );

			t1 = std::chrono::high_resolution_clock::now();
			if( const auto ret = prs_single->parse_file( bench_file_prp ); ret != PARSER_SUCCESS )
			{
				printf( "parser failed with: %s\n", code_to_str( ret ).c_str() );

				return 1;
			}
			t2 = std::chrono::high_resolution_clock::now();

			duration1 = std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 ).count();
			//duration2 = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 ).count();
			//printf( "[Benchmark-Parser-Single-Threaded] %lld microseconds, %lld milliseconds\n", duration1, duration2 );

			std::get< 1 >( results[ bench_file ] ) = std::get< 1 >( results[ bench_file ] ) + duration1;


			t1 = std::chrono::high_resolution_clock::now();
			if( const auto ret = prs_multi->parse_file_multithreaded_chunking( bench_file_prp ); ret != PARSER_SUCCESS )
			{
				printf( "parser failed with: %s\n", code_to_str( ret ).c_str() );

				return 1;
			}

			t2 = std::chrono::high_resolution_clock::now();

			duration1 = std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 ).count();
			//duration2 = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 ).count();
			//printf( "[Benchmark-Parser-Multi-Threaded][Threads=%d] %lld microseconds, %lld milliseconds\n",
			//        std::thread::hardware_concurrency(), duration1, duration2
			//);

			std::get< 2 >( results[ bench_file ] ) = std::get< 2 >( results[ bench_file ] ) + duration1;

			t1 = std::chrono::high_resolution_clock::now();
			if( const auto ret = interp->execute_on_env( "bench", *prs_multi->get_instructions(), "dummy.exe" ); ret !=
				INTERPRETER_SUCCESS )
			{
				printf( "interpreter failed with: %s\n", code_to_str( ret ).c_str() );

				return 1;
			}

			t2 = std::chrono::high_resolution_clock::now();

			duration1 = std::chrono::duration_cast< std::chrono::microseconds >( t2 - t1 ).count();
			//auto duration2 = std::chrono::duration_cast< std::chrono::milliseconds >( t2 - t1 ).count();
			//printf( "[Benchmark-Interpreter] %lld microseconds, %lld milliseconds\n", duration1, duration2 );

			std::get< 3 >( results[ bench_file ] ) = std::get< 3 >( results[ bench_file ] ) + duration1;
		}

		prs_single->clear_instructions();

		prs_multi->clear_instructions();
	}

	size_t idx = 0;

	for( const auto& [ bf, times ] : results )
	{
		const auto prp_avg_micros        = std::get< 0 >( times ) / REPS;
		const auto prs_single_avg_micros = std::get< 1 >( times ) / REPS;
		const auto prs_multi_avg_micros  = std::get< 2 >( times ) / REPS;
		const auto interp_avg_micros     = std::get< 3 >( times ) / REPS;

		const auto csv_entry = std::vformat( "{:d},{},{:d},{:d},{:d},{:d},{:d}",
		                                     std::make_format_args(
			                                     ++idx, bf.substr( bf.find( "sample_bench_" ), bf.find( ".ta" ) - 3 ),
			                                     REPS, prp_avg_micros, prs_single_avg_micros, prs_multi_avg_micros,
			                                     interp_avg_micros
		                                     )
		);

		results_csv.emplace_back( csv_entry );
	}

	if( Utils::write_as_file( "bench_results.csv", results_csv ) )
		printf( "[+] Wrote benchmarks to disk!\n" );
	else
	{
		// please dear god, never happen to me
		printf( "Failed to print benchmarks!!!!!!!!\n" );

		return 1;
	}

	printf( "[+] Benchmarks were run %d times\n", REPS );

	for( auto& dir_it : std::filesystem::directory_iterator( "..//Benchmarks" ) )
	{
		if( dir_it.is_directory() || dir_it.path().extension() != ".prp" )
			continue;

		std::filesystem::remove( dir_it );
	}

	printf( "[+] Deleted all .prp files.\n" );

	return 0;
}

void gen_hashes()
{
	const std::vector< std::string > insts = {
		"rpcae", "inc", "dec", "add", "sub", "mul", "div", "and", "or", "xor", "mov", "println", "jmp", "je", "jne",
		"jg", "jge", "jl", "jle", "sleep_ms", "if", "read_mem", "get_image_base", "get_image_size", "write_mem",
		"find_signature", "patch_bytes", "dump", "suspend_process", "resume_process", "inject_dll", "terminate_process",
		"show_threads_info", "show_images_info", "cmp"
	};

	printf( "generating hashes for %lld instructions\n", insts.size() );

	printf( "namespace instruction_hashes\n{\n" );

	constexpr std::hash< std::string > m_hash;

	for( const auto& elem : insts )
		printf( "\tconstexpr auto hash_%s = 0x%llX;\n", elem.c_str(), m_hash( elem ) );

	printf( "}\n" );
}

int main()
{
	//return test();
	return re_tool();
	//return benchmarking();

	return 0;
}
