/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <list>
#include <string>
#include <unordered_map>

#include "forward.h"

namespace blaze {

class Environment {
public:
	virtual ~Environment() = default;

	// Factory functions instead of constructors because it can fail in the bindings/arguments case
	static EnvironmentPtr create();
	static EnvironmentPtr create(EnvironmentPtr outer);
	static EnvironmentPtr create(const ASTNodePtr lambda, std::list<ASTNodePtr> arguments);

	bool exists(const std::string& symbol);
	ASTNodePtr set(const std::string& symbol, ASTNodePtr value);
	ASTNodePtr get(const std::string& symbol);

protected:
	Environment() {}

	EnvironmentPtr m_outer { nullptr };
	std::unordered_map<std::string, ASTNodePtr> m_values;
};

} // namespace blaze
