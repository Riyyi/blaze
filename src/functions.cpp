/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory> // std::static_pointer_cast
#include <string>

#include "ruc/format/color.h"
#include "ruc/format/format.h"

#include "ast.h"
#include "environment.h"
#include "error.h"
#include "forward.h"
#include "printer.h"
#include "types.h"
#include "util.h"

namespace blaze {

void GlobalEnvironment::add()
{
	auto add = [](std::list<ASTNodePtr> nodes) -> ASTNodePtr {
		int64_t result = 0;

		for (auto node : nodes) {
			if (!is<Number>(node.get())) {
				Error::the().addError(format("wrong argument type: number, '{}'", node));
				return nullptr;
			}

			result += std::static_pointer_cast<Number>(node)->number();
		}

		return makePtr<Number>(result);
	};

	m_values.emplace("+", makePtr<Function>(add));
}

void GlobalEnvironment::sub()
{
	auto sub = [](std::list<ASTNodePtr> nodes) -> ASTNodePtr {
		if (nodes.size() == 0) {
			return makePtr<Number>(0);
		}

		for (auto node : nodes) {
			if (!is<Number>(node.get())) {
				Error::the().addError(format("wrong argument type: number, '{}'", node));
				return nullptr;
			}
		}

		// Start with the first number
		int64_t result = std::static_pointer_cast<Number>(nodes.front())->number();

		// Skip the first node
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {
			result -= std::static_pointer_cast<Number>(*it)->number();
		}

		return makePtr<Number>(result);
	};

	m_values.emplace("-", makePtr<Function>(sub));
}

void GlobalEnvironment::mul()
{
	auto mul = [](std::list<ASTNodePtr> nodes) -> ASTNodePtr {
		int64_t result = 1;

		for (auto node : nodes) {
			if (!is<Number>(node.get())) {
				Error::the().addError(format("wrong argument type: number, '{}'", node));
				return nullptr;
			}

			result *= std::static_pointer_cast<Number>(node)->number();
		}

		return makePtr<Number>(result);
	};

	m_values.emplace("*", makePtr<Function>(mul));
}

void GlobalEnvironment::div()
{
	auto div = [this](std::list<ASTNodePtr> nodes) -> ASTNodePtr {
		if (nodes.size() == 0) {
			Error::the().addError(format("wrong number of arguments: {}, 0", m_current_key));
			return nullptr;
		}

		for (auto node : nodes) {
			if (!is<Number>(node.get())) {
				Error::the().addError(format("wrong argument type: number, '{}'", node));
				return nullptr;
			}
		}

		// Start with the first number
		double result = std::static_pointer_cast<Number>(nodes.front())->number();

		// Skip the first node
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {
			result /= std::static_pointer_cast<Number>(*it)->number();
		}

		return makePtr<Number>((int64_t)result);
	};

	m_values.emplace("/", makePtr<Function>(div));
}

// -----------------------------------------

#define NUMBER_COMPARE(symbol, comparison_symbol)                                                                \
	auto lambda = [this](std::list<ASTNodePtr> nodes) -> ASTNodePtr {                                            \
		bool result = true;                                                                                      \
                                                                                                                 \
		if (nodes.size() < 2) {                                                                                  \
			Error::the().addError(format("wrong number of arguments: {}, {}", m_current_key, nodes.size() - 1)); \
			return nullptr;                                                                                      \
		}                                                                                                        \
                                                                                                                 \
		for (auto node : nodes) {                                                                                \
			if (!is<Number>(node.get())) {                                                                       \
				Error::the().addError(format("wrong argument type: number, '{}'", node));                        \
				return nullptr;                                                                                  \
			}                                                                                                    \
		}                                                                                                        \
                                                                                                                 \
		/* Start with the first number */                                                                        \
		int64_t number = std::static_pointer_cast<Number>(nodes.front())->number();                              \
                                                                                                                 \
		/* Skip the first node */                                                                                \
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {                                      \
			int64_t current_number = std::static_pointer_cast<Number>(*it)->number();                            \
			if (number comparison_symbol current_number) {                                                       \
				result = false;                                                                                  \
				break;                                                                                           \
			}                                                                                                    \
			number = current_number;                                                                             \
		}                                                                                                        \
                                                                                                                 \
		return makePtr<Value>((result) ? Value::True : Value::False);                                            \
	};                                                                                                           \
                                                                                                                 \
	m_values.emplace(symbol, makePtr<Function>(lambda));

void GlobalEnvironment::lt()
{
	NUMBER_COMPARE("<", >=);
}

void GlobalEnvironment::lte()
{
	NUMBER_COMPARE("<=", >);
}

void GlobalEnvironment::gt()
{
	NUMBER_COMPARE(">", <=);
}

void GlobalEnvironment::gte()
{
	NUMBER_COMPARE(">=", <);
}

// -----------------------------------------

void GlobalEnvironment::list()
{
	auto list = [](std::list<ASTNodePtr> nodes) -> ASTNodePtr {
		auto list = makePtr<List>();

		for (auto node : nodes) {
			list->addNode(node);
		}

		return list;
	};

	m_values.emplace("list", makePtr<Function>(list));
}

void GlobalEnvironment::isList()
{
	auto is_list = [](std::list<ASTNodePtr> nodes) -> ASTNodePtr {
		bool result = true;

		for (auto node : nodes) {
			if (!is<List>(node.get())) {
				result = false;
				break;
			}
		}

		return makePtr<Value>((result) ? Value::True : Value::False);
	};

	m_values.emplace("list?", makePtr<Function>(is_list));
}

void GlobalEnvironment::isEmpty()
{
	auto is_empty = [](std::list<ASTNodePtr> nodes) -> ASTNodePtr {
		bool result = true;

		for (auto node : nodes) {
			if (!is<List>(node.get())) {
				Error::the().addError(format("wrong argument type: list, '{}'", node));
				return nullptr;
			}

			if (!std::static_pointer_cast<List>(node)->empty()) {
				result = false;
				break;
			}
		}

		return makePtr<Value>((result) ? Value::True : Value::False);
	};

	m_values.emplace("empty?", makePtr<Function>(is_empty));
}

void GlobalEnvironment::count()
{
	auto count = [this](std::list<ASTNodePtr> nodes) -> ASTNodePtr {
		if (nodes.size() != 1) {
			Error::the().addError(format("wrong number of arguments: {}, {}", m_current_key, nodes.size() - 1));
			return nullptr;
		}

		size_t result = 0;
		if (is<Value>(nodes.front().get()) && std::static_pointer_cast<Value>(nodes.front())->state() == Value::Nil) {
			// result = 0
		}
		else if (!is<List>(nodes.front().get())) {
			result = std::static_pointer_cast<List>(nodes.front())->size();
		}
		else if (!is<Vector>(nodes.front().get())) {
			result = std::static_pointer_cast<Vector>(nodes.front())->size();
		}
		else {
			Error::the().addError(format("wrong argument type: list, '{}'", nodes));
			return nullptr;
		}

		// FIXME: Add numeric_limits check for implicit cast: size_t > int64_t
		return makePtr<Number>((int64_t)result);
	};

	m_values.emplace("count", makePtr<Function>(count));
}

// -----------------------------------------

#define PRINTER_STRING(symbol, concatenation, print_readably)                       \
	auto lambda = [](std::list<ASTNodePtr> nodes) -> ASTNodePtr {                   \
		std::string result;                                                         \
                                                                                    \
		Printer printer;                                                            \
		for (auto it = nodes.begin(); it != nodes.end(); ++it) {                    \
			result += format("{}", printer.printNoErrorCheck(*it, print_readably)); \
                                                                                    \
			if (!isLast(it, nodes)) {                                               \
				result += concatenation;                                            \
			}                                                                       \
		}                                                                           \
                                                                                    \
		return makePtr<String>(result);                                             \
	};                                                                              \
                                                                                    \
	m_values.emplace(symbol, makePtr<Function>(lambda));

void GlobalEnvironment::str()
{
	PRINTER_STRING("str", "", false);
}

void GlobalEnvironment::prStr()
{
	PRINTER_STRING("pr-str", " ", true);
}

#define PRINTER_PRINT(symbol, print_readably)                            \
	auto lambda = [](std::list<ASTNodePtr> nodes) -> ASTNodePtr {        \
		Printer printer;                                                 \
		for (auto it = nodes.begin(); it != nodes.end(); ++it) {         \
			print("{}", printer.printNoErrorCheck(*it, print_readably)); \
                                                                         \
			if (!isLast(it, nodes)) {                                    \
				print(" ");                                              \
			}                                                            \
		}                                                                \
		print("\n");                                                     \
                                                                         \
		return makePtr<Value>(Value::Nil);                               \
	};                                                                   \
                                                                         \
	m_values.emplace(symbol, makePtr<Function>(lambda));

void GlobalEnvironment::prn()
{
	PRINTER_PRINT("prn", true);
}

void GlobalEnvironment::println()
{
	PRINTER_PRINT("println", false);
}

// -----------------------------------------

void GlobalEnvironment::equal()
{
	auto lambda = [this](std::list<ASTNodePtr> nodes) -> ASTNodePtr {
		if (nodes.size() < 2) {
			Error::the().addError(format("wrong number of arguments: {}, {}", m_current_key, nodes.size() - 1));
			return nullptr;
		}

		std::function<bool(ASTNodePtr, ASTNodePtr)> equal =
			[&equal](ASTNodePtr lhs, ASTNodePtr rhs) -> bool {
			if ((is<List>(lhs.get()) && is<List>(rhs.get()))
			    || (is<Vector>(lhs.get()) && is<Vector>(rhs.get()))) {
				auto lhs_nodes = std::static_pointer_cast<List>(lhs)->nodes();
				auto rhs_nodes = std::static_pointer_cast<List>(rhs)->nodes();

				if (lhs_nodes.size() != rhs_nodes.size()) {
					return false;
				}

				auto lhs_it = lhs_nodes.begin();
				auto rhs_it = rhs_nodes.begin();
				for (; lhs_it != lhs_nodes.end(); ++lhs_it, ++rhs_it) {
					if (!equal(*lhs_it, *rhs_it)) {
						return false;
					}
				}

				return true;
			}

			if (is<HashMap>(lhs.get()) && is<HashMap>(rhs.get())) {
				auto lhs_nodes = std::static_pointer_cast<HashMap>(lhs)->elements();
				auto rhs_nodes = std::static_pointer_cast<HashMap>(rhs)->elements();

				if (lhs_nodes.size() != rhs_nodes.size()) {
					return false;
				}

				for (const auto& [key, value] : lhs_nodes) {
					auto it = rhs_nodes.find(key);
					if (it == rhs_nodes.end() || !equal(value, it->second)) {
						return false;
					}
				}

				return true;
			}

			if (is<String>(lhs.get()) && is<String>(rhs.get())
			    && std::static_pointer_cast<String>(lhs)->data() == std::static_pointer_cast<String>(rhs)->data()) {
				return true;
			}
			if (is<Keyword>(lhs.get()) && is<Keyword>(rhs.get())
			    && std::static_pointer_cast<Keyword>(lhs)->keyword() == std::static_pointer_cast<Keyword>(rhs)->keyword()) {
				return true;
			}
			if (is<Number>(lhs.get()) && is<Number>(rhs.get())
			    && std::static_pointer_cast<Number>(lhs)->number() == std::static_pointer_cast<Number>(rhs)->number()) {
				return true;
			}
			if (is<Value>(lhs.get()) && is<Value>(rhs.get())
			    && std::static_pointer_cast<Value>(lhs)->state() == std::static_pointer_cast<Value>(rhs)->state()) {
				return true;
			}
			if (is<Symbol>(lhs.get()) && is<Symbol>(rhs.get())
			    && std::static_pointer_cast<Symbol>(lhs)->symbol() == std::static_pointer_cast<Symbol>(rhs)->symbol()) {
				return true;
			}

			return false;
		};

		bool result = true;
		auto it = nodes.begin();
		auto it_next = std::next(nodes.begin());
		for (; it_next != nodes.end(); ++it, ++it_next) {
			if (!equal(*it, *it_next)) {
				result = false;
				break;
			}
		}

		return makePtr<Value>((result) ? Value::True : Value::False);
	};

	m_values.emplace("=", makePtr<Function>(lambda));
}

} // namespace blaze
