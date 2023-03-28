/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include "ruc/format/print.h"

#include "ast.h"
#include "environment.h"

namespace blaze {

Environment::Environment(EnvironmentPtr outer)
	: m_outer(outer)
{
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
