/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory> // std::static_pointer_cast

#include "ruc/format/format.h"

#include "ast.h"
#include "env/environment.h"
#include "error.h"
#include "forward.h"

namespace blaze {

std::unordered_map<std::string, FunctionType> Environment::s_functions;

EnvironmentPtr Environment::create()
{
	return std::shared_ptr<Environment>(new Environment);
}

EnvironmentPtr Environment::create(EnvironmentPtr outer)
{
	auto env = create();

	env->m_outer = outer;

	return env;
}

EnvironmentPtr Environment::create(const ValuePtr lambda, ValueVector&& arguments)
{
	auto lambda_casted = std::static_pointer_cast<Lambda>(lambda);
	auto env = create(lambda_casted->env());
	auto bindings = lambda_casted->bindings();

	auto it = arguments.begin();
	for (size_t i = 0; i < bindings.size(); ++i, ++it) {
		if (bindings[i] == "&") {
			if (i + 2 != bindings.size()) {
				Error::the().add(::format("invalid function: {}", lambda));
				return nullptr;
			}

			auto nodes = ValueVector();
			for (; it != arguments.end(); ++it) {
				nodes.push_back(*it);
			}
			env->set(bindings[i + 1], makePtr<List>(nodes));

			return env;
		}

		if (it == arguments.end()) {
			Error::the().add(::format("wrong number of arguments: {}, {}", lambda, arguments.size()));
			return nullptr;
		}

		env->set(bindings[i], *it);
	}

	if (it != arguments.end()) {
		Error::the().add(::format("wrong number of arguments: {}, {}", lambda, arguments.size()));
		return nullptr;
	}

	return env;
}

// -----------------------------------------

void Environment::registerFunction(const std::string& name, FunctionType function)
{
	s_functions.insert_or_assign(name, function);
}

void Environment::installFunctions(EnvironmentPtr env)
{
	for (const auto& [name, function] : s_functions) {
		env->set(name, makePtr<Function>(name, function));
	}
}

// -----------------------------------------

bool Environment::exists(const std::string& symbol)
{
	return m_values.find(symbol) != m_values.end();
}

ValuePtr Environment::set(const std::string& symbol, ValuePtr value)
{
	if (exists(symbol)) {
		m_values.erase(symbol);
	}

	m_values.emplace(symbol, value);

	return value;
}

ValuePtr Environment::get(const std::string& symbol)
{
	if (exists(symbol)) {
		return m_values[symbol];
	}

	if (m_outer) {
		return m_outer->get(symbol);
	}

	return nullptr;
}

} // namespace blaze
