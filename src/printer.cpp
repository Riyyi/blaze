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
#include "util.h"

namespace blaze {

Printer::Printer()
{
}

Printer::~Printer()
{
}

// -----------------------------------------

std::string Printer::print(ValuePtr node, bool print_readably)
{
	if (Error::the().hasAnyError()) {
		init();
		printError();
		return m_print;
	}

	return printNoErrorCheck(node, print_readably);
}

std::string Printer::printNoErrorCheck(ValuePtr node, bool print_readably)
{
	init();

	if (node == nullptr) {
		return {};
	}

	printImpl(node, print_readably);

	return m_print;
}

// -----------------------------------------

void Printer::init()
{
	m_first_node = true;
	m_previous_node_is_list = false;
	m_print = "";
}

void Printer::printImpl(ValuePtr node, bool print_readably)
{
	auto printSpacing = [this]() -> void {
		if (!m_first_node && !m_previous_node_is_list) {
			m_print += ' ';
		}
	};

	Value* node_raw_ptr = node.get();
	if (is<Collection>(node_raw_ptr)) {
		printSpacing();
		m_print += (is<List>(node_raw_ptr)) ? '(' : '[';
		m_first_node = false;
		m_previous_node_is_list = true;
		auto nodes = std::static_pointer_cast<Collection>(node)->nodes();
		for (auto node : nodes) {
			printImpl(node, print_readably);
			m_previous_node_is_list = false;
		}
		m_print += (is<List>(node_raw_ptr)) ? ')' : ']';
	}
	else if (is<HashMap>(node_raw_ptr)) {
		printSpacing();
		m_print += "{";
		m_first_node = false;
		m_previous_node_is_list = true;
		auto elements = std::static_pointer_cast<HashMap>(node)->elements();
		for (auto it = elements.begin(); it != elements.end(); ++it) {
			m_print += ::format("{} ", (it->first.front() == 0x7f) ? ":" + it->first.substr(1) : '"' + it->first + '"'); // 127
			printImpl(it->second, print_readably);

			if (!isLast(it, elements)) {
				m_print += ' ';
			}
		}
		m_previous_node_is_list = false;
		m_print += '}';
	}
	else if (is<String>(node_raw_ptr)) {
		std::string text = std::static_pointer_cast<String>(node)->data();
		if (print_readably) {
			text = replaceAll(text, "\\", "\\\\");
			text = replaceAll(text, "\"", "\\\"");
			text = replaceAll(text, "\n", "\\n");
			text = "\"" + text + "\"";
		}
		printSpacing();
		m_print += ::format("{}", text);
	}
	else if (is<Keyword>(node_raw_ptr)) {
		printSpacing();
		m_print += ::format(":{}", std::static_pointer_cast<Keyword>(node)->keyword().substr(1));
	}
	else if (is<Number>(node_raw_ptr)) {
		printSpacing();
		m_print += ::format("{}", std::static_pointer_cast<Number>(node)->number());
	}
	else if (is<Constant>(node_raw_ptr)) {
		printSpacing();
		std::string value;
		switch (std::static_pointer_cast<Constant>(node)->state()) {
		case Constant::Nil: value = "nil"; break;
		case Constant::True: value = "true"; break;
		case Constant::False: value = "false"; break;
		}
		m_print += ::format("{}", value);
	}
	else if (is<Symbol>(node_raw_ptr)) {
		printSpacing();
		m_print += ::format("{}", std::static_pointer_cast<Symbol>(node)->symbol());
	}
	else if (is<Function>(node_raw_ptr)) {
		printSpacing();
		m_print += ::format("#<builtin-function>({})", std::static_pointer_cast<Function>(node)->name());
	}
	else if (is<Lambda>(node_raw_ptr)) {
		printSpacing();
		m_print += ::format("#<user-function>({:p})", node_raw_ptr);
	}
	else if (is<Macro>(node_raw_ptr)) {
		printSpacing();
		m_print += ::format("#<user-macro>({:p})", node_raw_ptr);
	}
	else if (is<Atom>(node_raw_ptr)) {
		printSpacing();
		m_print += "(atom ";
		printImpl(std::static_pointer_cast<Atom>(node)->deref(), print_readably);
		m_print += ")";
	}
}

void Printer::printError()
{
	m_print = "Error: ";
	if (Error::the().hasTokenError()) {
		Token error = Error::the().tokenError();
		m_print += ::format("{}", error.symbol);
	}
	else if (Error::the().hasOtherError()) {
		std::string error = Error::the().otherError();
		m_print += ::format("{}", error);
	}
	else if (Error::the().hasException()) {
		ValuePtr error = Error::the().exception();
		m_print += ::format("{}", error);
	}
}

} // namespace blaze
