#pragma once
#include <memory>

#include "instruction.hpp"

class inst_get_image_base : public instruction
{
public:

	inst_get_image_base( const std::string & name, const std::string& vd ) : m_image_name{ name }, m_vdr{ vd } {}

	void evaluate(std::shared_ptr<environment> env) override;

private:
	std::string m_image_name, m_vdr;
};