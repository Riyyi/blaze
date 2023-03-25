/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdint> // int64_t
#include <string>

#include "ast.h"
#include "printer.h"
#include "types.h"

namespace blaze {

Collection::~Collection()
{
	for (auto node : m_nodes) {
		delete node;
	}
}

void Collection::addNode(ASTNode* node)
{
	m_nodes.push_back(node);
}

// -----------------------------------------

void HashMap::addElement(const std::string& key, ASTNode* value)
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

Value::Value(const std::string& value)
	: m_value(value)
{
}

// -----------------------------------------

Function::Function(Lambda lambda)
	: m_lambda(lambda)
{
}

} // namespace blaze

// -----------------------------------------

void Formatter<blaze::ASTNode*>::format(Builder& builder, blaze::ASTNode* value) const
{
	blaze::Printer printer;
	return Formatter<std::string>::format(builder, printer.printNoErrorCheck(value));
}
