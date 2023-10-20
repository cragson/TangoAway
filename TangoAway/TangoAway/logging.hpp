#pragma once
#include <cstdarg>
#include <cstdio>
#include <chrono>
#include <format>

// Enable different log level by uncommenting
//#define TANGO_AWAY_DEBUG
//#define TANGO_AWAY_INFO

namespace Log
{
	inline std::string get_timestamp()
	{
		const static auto cet = std::chrono::locate_zone( "Etc/GMT-2" );

		return std::vformat( "{:%d.%m.%y %H:%M:%S}", std::make_format_args( std::chrono::zoned_time{
				                     cet, floor< std::chrono::seconds >( std::chrono::system_clock::now() )
			                     }
		                     )
		);
	}

	inline void log( int32_t level, const char* fmt, ... )
	{
		va_list param_ptr;
		va_start( param_ptr, fmt );

		auto ts = std::string( "[" );

		ts += get_timestamp();
		ts += "] ";

		if( level == 0 )
			ts += "[DEBUG] ";

		if( level == 1 )
			ts += "[INFO] ";

		if( level == 2 )
			ts += "[TEST] ";

		ts += fmt;

		vfprintf( stdout, ts.c_str(), param_ptr );
		va_end( param_ptr );
	}

	template< typename ... Args >
	inline void debug( const char* fmt, Args ... args )
	{
#ifdef TANGO_AWAY_DEBUG

		log( 0, fmt, args ... );

#endif
	}

	template< typename ... Args >
	inline void info( const char* fmt, Args ... args )
	{
#ifdef TANGO_AWAY_INFO

		log( 1, fmt, args ... );

#endif
	}

	template< typename ... Args >
	inline void test( const char* fmt, Args ... args )
	{
		log( 2, fmt, args ... );
	}
}
