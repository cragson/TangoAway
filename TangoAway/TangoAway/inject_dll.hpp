#pragma once
#include <memory>

#include "instruction.hpp"

class inst_inject_dll : public instruction
{
public:
	inst_inject_dll() = delete;

	inst_inject_dll( const std::string& path, const std::string& mth )
		: m_dll_path{ path }
		, m_method{ mth } {}

	void evaluate( std::shared_ptr< environment > env ) override;

private:
	std::string m_dll_path;
	std::string m_method;
};
