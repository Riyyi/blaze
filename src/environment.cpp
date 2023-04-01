/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include "ruc/format/print.h"

#include "ast.h"
#include "environment.h"
#include "error.h"
#include "forward.h"

namespace blaze {

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

EnvironmentPtr Environment::create(EnvironmentPtr outer, std::vector<std::string> bindings, std::list<ASTNodePtr> arguments)
{
	auto env = create(outer);

	if (bindings.size() != arguments.size()) {
		Error::the().addError(format("wrong number of arguments: fn*, {}", arguments.size()));
		return nullptr;
	}

	auto bindings_it = bindings.begin();
	auto arguments_it = arguments.begin();
	for (; bindings_it != bindings.end(); ++bindings_it, ++arguments_it) {
		env->m_values.emplace(*bindings_it, *arguments_it);
	}

	return env;
}

// -----------------------------------------

bool Environment::exists(const std::string& symbol)
{
	return m_values.find(symbol) != m_values.end();
}

ASTNodePtr Environment::set(const std::string& symbol, ASTNodePtr value)
{
	if (exists(symbol)) {
		m_values.erase(symbol);
	}

	m_values.emplace(symbol, value);

	return value;
}

ASTNodePtr Environment::get(const std::string& symbol)
{
	m_current_key = symbol;

	if (exists(symbol)) {
		return m_values[symbol];
	}

	if (m_outer) {
		return m_outer->get(symbol);
	}

	return nullptr;
}

// -----------------------------------------

GlobalEnvironment::GlobalEnvironment()
{
	add();
	sub();
	mul();
	div();

	lt();
	lte();
	gt();
	gte();

	list();
	isList();
	isEmpty();
	count();

	str();
	prStr();
	prn();
	println();

	equal();
}

} // namespace blaze
