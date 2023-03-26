/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <unordered_map>

#include "ast.h"

namespace blaze {

class Environment;
typedef std::shared_ptr<Environment> EnvironmentPtr;

class Environment {
public:
	Environment() = default;
	Environment(EnvironmentPtr outer);
	virtual ~Environment() = default;

	bool exists(const std::string& symbol);
	ASTNodePtr set(const std::string& symbol, ASTNodePtr value);
	ASTNodePtr get(const std::string& symbol);

protected:
	std::string m_current_key;
	std::unordered_map<std::string, ASTNodePtr> m_values;
	EnvironmentPtr m_outer { nullptr };
};

class GlobalEnvironment final : public Environment {
public:
	GlobalEnvironment();
	virtual ~GlobalEnvironment() = default;

private:
	// TODO: Add more native functions
	void add();
	void sub();
	void mul();
	void div();
};

} // namespace blaze
