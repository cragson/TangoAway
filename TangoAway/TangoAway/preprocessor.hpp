#pragma once
#include <string>

#include "status_code.hpp"

#ifndef PREPROCESSOR_REMOVE_LINE
#define PREPROCESSOR_REMOVE_LINE "@>REMOVE<@"
#endif

#ifndef PREPROCESSOR_MACRO_PARAM_STATIC
#define PREPROCESSOR_MACRO_PARAM_STATIC "@"
#endif

class preprocessor
{
public:
	[[nodiscard]] EStatusCode process_file( const std::string& path_to_file );

	[[nodiscard]] std::string make_macro_parameter_marker( const size_t parameter_index ) const;
};
