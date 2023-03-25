/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <iterator> // std::next
#include <string>

#include "ruc/format/format.h"

#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "printer.h"
#include "types.h"

namespace blaze {

Printer::Printer()
{
}

Printer::~Printer()
{
}

// -----------------------------------------

std::string Printer::print(ASTNode* node)
{
	if (Error::the().hasAnyError()) {
		init();
		printError();
		return m_print;
	}

	return printNoErrorCheck(node);
}

std::string Printer::printNoErrorCheck(ASTNode* node)
{
	init();

	if (node == nullptr) {
		return {};
	}

	printImpl(node);

	return m_print;
}

// -----------------------------------------

void Printer::init()
{
	m_first_node = true;
	m_previous_node_is_list = false;
	m_print = "";
}

void Printer::printImpl(ASTNode* node)
{
	auto printSpacing = [this]() -> void {
		if (!m_first_node && !m_previous_node_is_list) {
			m_print += ' ';
		}
	};

	if (is<List>(node)) {
		printSpacing();
		m_print += '(';
		m_first_node = false;
		m_previous_node_is_list = true;
		auto nodes = static_cast<List*>(node)->nodes();
		for (size_t i = 0; i < nodes.size(); ++i) {
			printImpl(nodes[i]);
			m_previous_node_is_list = false;
		}
		m_print += ')';
	}
	else if (is<Vector>(node)) {
		printSpacing();
		m_print += '[';
		m_first_node = false;
		m_previous_node_is_list = true;
		auto nodes = static_cast<Vector*>(node)->nodes();
		for (size_t i = 0; i < nodes.size(); ++i) {
			printImpl(nodes[i]);
			m_previous_node_is_list = false;
		}
		m_print += ']';
	}
	else if (is<HashMap>(node)) {
		printSpacing();
		m_print += "{";
		m_first_node = false;
		m_previous_node_is_list = true;
		auto elements = static_cast<HashMap*>(node)->elements();
		for (auto it = elements.begin(); it != elements.end(); ++it) {
			m_print += format("{} ", it->first.front() == 0x7f ? ":" + it->first.substr(1) : it->first); // 127
			printImpl(it->second);

			if (it != elements.end() && std::next(it) != elements.end()) {
				m_print += ' ';
			}
		}
		m_previous_node_is_list = false;
		m_print += '}';
	}
	else if (is<String>(node)) {
		// TODO: Implement string readably printing
		printSpacing();
		m_print += format("{}", static_cast<String*>(node)->data());
	}
	else if (is<Keyword>(node)) {
		printSpacing();
		m_print += format(":{}", static_cast<Keyword*>(node)->keyword().substr(1));
	}
	else if (is<Number>(node)) {
		printSpacing();
		m_print += format("{}", static_cast<Number*>(node)->number());
	}
	else if (is<Symbol>(node)) {
		printSpacing();
		m_print += format("{}", static_cast<Symbol*>(node)->symbol());
	}
}

void Printer::printError()
{
	m_print = "Error: ";
	if (Error::the().hasTokenError()) {
		Token error = Error::the().tokenError();
		m_print += format("{}", error.symbol);
	}
	else if (Error::the().hasOtherError()) {
		std::string error = Error::the().otherError();
		m_print += format("{}", error);
	}
}

} // namespace blaze
