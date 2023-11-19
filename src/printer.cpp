/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <iterator> // std::next
#include <memory>   // std::static_pointer_cast
#include <string>

#include "ruc/format/color.h"
#include "ruc/format/format.h"

#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "printer.h"
#include "settings.h"
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

std::string Printer::print(ValuePtr value, bool print_readably)
{
	if (Error::the().hasAnyError()) {
		init();
		printError();
		return m_print;
	}

	return printNoErrorCheck(value, print_readably);
}

std::string Printer::printNoErrorCheck(ValuePtr value, bool print_readably)
{
	init();

	if (value == nullptr) {
		return {};
	}

	printImpl(value, print_readably);

	return m_print;
}

// -----------------------------------------

void Printer::init()
{
	m_first_node = true;
	m_previous_node_is_list = false;
	m_print = "";
}

void Printer::printImpl(ValuePtr value, bool print_readably)
{
	bool pretty_print = Settings::the().getEnvBool("*PRETTY-PRINT*");

	auto printSpacing = [this]() -> void {
		if (!m_first_node && !m_previous_node_is_list) {
			m_print += ' ';
		}
	};

	Value* value_raw_ptr = value.get();
	if (is<Collection>(value_raw_ptr)) {
		printSpacing();
		m_print += (is<List>(value_raw_ptr)) ? '(' : '[';
		m_first_node = false;
		m_previous_node_is_list = true;
		auto nodes = std::static_pointer_cast<Collection>(value)->nodesRead();
		for (auto node : nodes) {
			printImpl(node, print_readably);
			m_previous_node_is_list = false;
		}
		m_print += (is<List>(value_raw_ptr)) ? ')' : ']';
	}
	else if (is<HashMap>(value_raw_ptr)) {
		printSpacing();
		m_print += "{";
		m_first_node = false;
		m_previous_node_is_list = true;
		auto elements = std::static_pointer_cast<HashMap>(value)->elements();
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
	else if (is<String>(value_raw_ptr)) {
		std::string text = std::static_pointer_cast<String>(value)->data();
		if (print_readably) {
			text = replaceAll(text, "\\", "\\\\");
			text = replaceAll(text, "\"", "\\\"");
			text = replaceAll(text, "\n", "\\n");
			text = "\"" + text + "\"";
		}
		printSpacing();
		if (pretty_print) {
			m_print += ::format(fg(ruc::format::TerminalColor::BrightGreen), "{}", text);
		}
		else {
			m_print += ::format("{}", text);
		}
	}
	else if (is<Keyword>(value_raw_ptr)) {
		printSpacing();
		m_print += ::format(":{}", std::static_pointer_cast<Keyword>(value)->keyword().substr(1));
	}
	else if (is<Number>(value_raw_ptr)) {
		printSpacing();
		m_print += ::format("{}", std::static_pointer_cast<Number>(value)->number());
	}
	else if (is<Constant>(value_raw_ptr)) {
		printSpacing();
		std::string constant;
		switch (std::static_pointer_cast<Constant>(value)->state()) {
		case Constant::Nil: constant = "nil"; break;
		case Constant::True: constant = "true"; break;
		case Constant::False: constant = "false"; break;
		}
		m_print += ::format("{}", constant);
	}
	else if (is<Symbol>(value_raw_ptr)) {
		printSpacing();
		m_print += ::format("{}", std::static_pointer_cast<Symbol>(value)->symbol());
	}
	else if (is<Function>(value_raw_ptr)) {
		printSpacing();
		m_print += ::format("#<builtin-function>({})", std::static_pointer_cast<Function>(value)->name());
	}
	else if (is<Lambda>(value_raw_ptr)) {
		printSpacing();
		m_print += ::format("#<user-function>({:p})", value_raw_ptr);
	}
	else if (is<Macro>(value_raw_ptr)) {
		printSpacing();
		m_print += ::format("#<user-macro>({:p})", value_raw_ptr);
	}
	else if (is<Atom>(value_raw_ptr)) {
		printSpacing();
		m_print += "(atom ";
		printImpl(std::static_pointer_cast<Atom>(value)->deref(), print_readably);
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
