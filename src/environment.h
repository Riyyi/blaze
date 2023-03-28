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
	void add(); // +
	void sub(); // -
	void mul(); // *
	void div(); // /

	void lt();  // <
	void lte(); // <=
	void gt();  // >
	void gte(); // >=

	void list();    // list
	void isList();  // list?
	void isEmpty(); // empty?
	void count();   // count

	void str();     // str
	void prStr();   // pr-str
	void prn();     // prn
	void println(); // println

	void equal(); // =
};

} // namespace blaze
