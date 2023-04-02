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

void Collection::add(ASTNodePtr node)
{
	m_nodes.push_back(node);
}

// -----------------------------------------

void HashMap::addElement(const std::string& key, ASTNodePtr value)
{
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

Value::Value(State state)
	: m_state(state)
{
}

// -----------------------------------------

Function::Function(FunctionType function)
	: m_function(function)
{
}

// -----------------------------------------

Lambda::Lambda(std::vector<std::string> bindings, ASTNodePtr body, EnvironmentPtr env)
	: m_bindings(bindings)
	, m_body(body)
	, m_env(env)
{
}

} // namespace blaze

// -----------------------------------------

void Formatter<blaze::ASTNodePtr>::format(Builder& builder, blaze::ASTNodePtr value) const
{
	blaze::Printer printer;
	return Formatter<std::string>::format(builder, printer.printNoErrorCheck(value));
}
