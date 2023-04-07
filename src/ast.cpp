/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdint> // int64_t
#include <memory>
#include <string>

#include "ast.h"
#include "environment.h"
#include "forward.h"
#include "printer.h"
#include "types.h"

namespace blaze {

void Collection::add(ValuePtr node)
{
	if (node == nullptr) {
		return;
	}

	m_nodes.push_back(node);
}

// -----------------------------------------

void HashMap::add(const std::string& key, ValuePtr value)
{
	if (value == nullptr) {
		return;
	}

	m_elements.emplace(key, value);
}

// -----------------------------------------

String::String(const std::string& data)
	: m_data(data)
{
}

// -----------------------------------------

Keyword::Keyword(const std::string& data)
	: m_data(data)
{
}

// -----------------------------------------

Number::Number(int64_t number)
	: m_number(number)
{
}

// -----------------------------------------

Symbol::Symbol(const std::string& symbol)
	: m_symbol(symbol)
{
}

// -----------------------------------------

Constant::Constant(State state)
	: m_state(state)
{
}

// -----------------------------------------

Function::Function(const std::string& name, FunctionType function)
	: m_name(name)
	, m_function(function)
{
}

// -----------------------------------------

Lambda::Lambda(std::vector<std::string> bindings, ValuePtr body, EnvironmentPtr env)
	: m_bindings(bindings)
	, m_body(body)
	, m_env(env)
{
}

// -----------------------------------------

Atom::Atom(ValuePtr pointer)
	: m_value(pointer)
{
}

} // namespace blaze

// -----------------------------------------

void Formatter<blaze::ValuePtr>::format(Builder& builder, blaze::ValuePtr value) const
{
	blaze::Printer printer;
	return Formatter<std::string>::format(builder, printer.printNoErrorCheck(value));
}
