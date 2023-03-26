/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <iterator> // std::next
#include <memory>   // std::static_pointer_cast
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

std::string Printer::print(ASTNodePtr node)
{
	if (Error::the().hasAnyError()) {
		init();
		printError();
		return m_print;
	}

	return printNoErrorCheck(node);
}

std::string Printer::printNoErrorCheck(ASTNodePtr node)
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

void Printer::printImpl(ASTNodePtr node)
{
	auto printSpacing = [this]() -> void {
		if (!m_first_node && !m_previous_node_is_list) {
			m_print += ' ';
		}
	};

	ASTNode* node_raw_ptr = node.get();
	if (is<List>(node_raw_ptr)) {
		printSpacing();
		m_print += '(';
		m_first_node = false;
		m_previous_node_is_list = true;
		auto nodes = std::static_pointer_cast<List>(node)->nodes();
		for (size_t i = 0; i < nodes.size(); ++i) {
			printImpl(nodes[i]);
			m_previous_node_is_list = false;
		}
		m_print += ')';
	}
	else if (is<Vector>(node_raw_ptr)) {
		printSpacing();
		m_print += '[';
		m_first_node = false;
		m_previous_node_is_list = true;
		auto nodes = std::static_pointer_cast<Vector>(node)->nodes();
		for (size_t i = 0; i < nodes.size(); ++i) {
			printImpl(nodes[i]);
			m_previous_node_is_list = false;
		}
		m_print += ']';
	}
	else if (is<HashMap>(node_raw_ptr)) {
		printSpacing();
		m_print += "{";
		m_first_node = false;
		m_previous_node_is_list = true;
		auto elements = std::static_pointer_cast<HashMap>(node)->elements();
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
	else if (is<String>(node_raw_ptr)) {
		// TODO: Implement string readably printing
		printSpacing();
		m_print += format("{}", std::static_pointer_cast<String>(node)->data());
	}
	else if (is<Keyword>(node_raw_ptr)) {
		printSpacing();
		m_print += format(":{}", std::static_pointer_cast<Keyword>(node)->keyword().substr(1));
	}
	else if (is<Number>(node_raw_ptr)) {
		printSpacing();
		m_print += format("{}", std::static_pointer_cast<Number>(node)->number());
	}
	else if (is<Symbol>(node_raw_ptr)) {
		printSpacing();
		m_print += format("{}", std::static_pointer_cast<Symbol>(node)->symbol());
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
