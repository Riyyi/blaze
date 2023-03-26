/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory> // std::static_pointer_cast

#include "ruc/format/format.h"

#include "ast.h"
#include "environment.h"
#include "error.h"
#include "types.h"

namespace blaze {

void GlobalEnvironment::add()
{
	auto add = [](std::span<ASTNodePtr> nodes) -> ASTNodePtr {
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
	auto sub = [](std::span<ASTNodePtr> nodes) -> ASTNodePtr {
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
		int64_t result = std::static_pointer_cast<Number>(nodes[0])->number();

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
	auto mul = [](std::span<ASTNodePtr> nodes) -> ASTNodePtr {
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
	auto div = [this](std::span<ASTNodePtr> nodes) -> ASTNodePtr {
		if (nodes.size() == 0) {
			Error::the().addError(format("wrong number of arguments: {}, 0", m_current_key));
			return nullptr;
		}

		for (auto node : nodes) {
			if (!is<Number>(node.get())) {
				Error::the().addError(format("wrong type argument: number-or-marker-p, '{}'", node));
				return nullptr;
			}
		}

		// Start with the first number
		double result = std::static_pointer_cast<Number>(nodes[0])->number();

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
	auto lambda = [this](std::span<ASTNodePtr> nodes) -> ASTNodePtr {                                            \
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
		int64_t number = std::static_pointer_cast<Number>(nodes[0])->number();                                   \
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
	auto list = [](std::span<ASTNodePtr> nodes) -> ASTNodePtr {
		auto list = makePtr<List>();

		for (auto node : nodes) {
			list->addNode(node);
		}

		return list;
	};

	m_values.emplace("list", makePtr<Function>(list));
}

void GlobalEnvironment::is_list()
{
	auto is_list = [](std::span<ASTNodePtr> nodes) -> ASTNodePtr {
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

void GlobalEnvironment::is_empty()
{
	auto is_empty = [](std::span<ASTNodePtr> nodes) -> ASTNodePtr {
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
	auto count = [this](std::span<ASTNodePtr> nodes) -> ASTNodePtr {
		if (nodes.size() > 1) {
			Error::the().addError(format("wrong number of arguments: {}, {}", m_current_key, nodes.size() - 1));
			return nullptr;
		}

		size_t result = 0;

		for (auto node : nodes) {
			if (!is<List>(node.get())) {
				Error::the().addError(format("wrong argument type: list, '{}'", node));
				return nullptr;
			}

			result = std::static_pointer_cast<List>(node)->size();
		}

		// FIXME: Add numeric_limits check for implicit cast: size_t > int64_t
		return makePtr<Number>((int64_t)result);
	};

	m_values.emplace("count", makePtr<Function>(count));
}

} // namespace blaze
