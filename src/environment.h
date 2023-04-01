/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <list>
#include <string>
#include <unordered_map>
#include <vector>

#include "badge.h"
#include "forward.h"

namespace blaze {

class Environment {
public:
	virtual ~Environment() = default;

	// Factory functions instead of constructors because it can fail in the bindings/arguments case
	static EnvironmentPtr create();
	static EnvironmentPtr create(EnvironmentPtr outer);
	static EnvironmentPtr create(EnvironmentPtr outer, std::vector<std::string> bindings, std::list<ASTNodePtr> arguments);

	bool exists(const std::string& symbol);
	ASTNodePtr set(const std::string& symbol, ASTNodePtr value);
	ASTNodePtr get(const std::string& symbol);

protected:
	Environment() {}

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
