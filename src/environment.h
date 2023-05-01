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
	static EnvironmentPtr create(const ValuePtr lambda, const ValueList& arguments);

	bool exists(const std::string& symbol);
	ValuePtr set(const std::string& symbol, ValuePtr value);
	ValuePtr get(const std::string& symbol);

protected:
	Environment() {}

	EnvironmentPtr m_outer { nullptr };
	std::unordered_map<std::string, ValuePtr> m_values;
};

} // namespace blaze
