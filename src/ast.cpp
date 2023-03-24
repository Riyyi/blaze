/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdint> // int64_t
#include <string>

#include "ast.h"

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

} // namespace blaze
