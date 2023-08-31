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

#include "ast.h"
#include "forward.h"

namespace blaze {

class Environment {
public:
	virtual ~Environment() = default;

	// Factory functions instead of constructors because it can fail in the bindings/arguments case
	static EnvironmentPtr create();
	static EnvironmentPtr create(EnvironmentPtr outer);
	static EnvironmentPtr create(const ValuePtr lambda, ValueVector&& arguments);

	static void loadFunctions();
	static void registerFunction(const std::string& name, FunctionType function);
	static void installFunctions(EnvironmentPtr env);

	bool exists(const std::string& symbol);
	ValuePtr set(const std::string& symbol, ValuePtr value);
	ValuePtr get(const std::string& symbol);

private:
	Environment() {}

	// Outer environment native functions, "Core"
	static void loadCollectionAccess();
	static void loadCollectionConstructor();
	static void loadCollectionModify();
	static void loadCompare();
	static void loadConvert();
	static void loadFormat();
	static void loadMeta();
	static void loadMutable();
	static void loadOperators();
	static void loadOther();
	static void loadPredicate();
	static void loadRepl();

	EnvironmentPtr m_outer { nullptr };
	std::unordered_map<std::string, ValuePtr> m_values;

	static std::unordered_map<std::string, FunctionType> s_functions;
	static std::vector<std::string> s_lambdas;
};

} // namespace blaze
