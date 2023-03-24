/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <cstdint> // int64_t
#include <iostream>
#include <span>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "error.h"
#include "ruc/format/color.h"
#include "ruc/singleton.h"

#include "ast.h"
#include "types.h"

namespace blaze {

class Environment {
public:
	Environment() = default;
	virtual ~Environment() = default;

	ASTNode* lookup(const std::string& symbol)
	{
		m_current_key = symbol;
		return m_values.find(symbol) != m_values.end() ? m_values[symbol] : nullptr;
	}

protected:
	std::string m_current_key;
	std::unordered_map<std::string, ASTNode*> m_values;
};

class GlobalEnvironment final : public Environment {
public:
	GlobalEnvironment()
	{
		// TODO: Add more native functions
		// TODO: Move the functions to their own file
		auto add = [](std::span<ASTNode*> nodes) -> ASTNode* {
			int64_t result = 0;

			for (auto node : nodes) {
				if (!is<Number>(node)) {
					Error::the().addError(format("wrong type argument: number-or-marker-p, '{}'", node));
					return nullptr;
				}

				result += static_cast<Number*>(node)->number();
			}

			return new Number(result);
		};

		auto sub = [](std::span<ASTNode*> nodes) -> ASTNode* {
			int64_t result = 0;

			if (nodes.size() == 0) {
				return new Number(0);
			}

			for (auto node : nodes) {
				if (!is<Number>(node)) {
					Error::the().addError(format("wrong type argument: number-or-marker-p, '{}'", node));
					return nullptr;
				}
			}

			// Start with the first number
			result += static_cast<Number*>(nodes[0])->number();

			// Skip the first node
			for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {
				result -= static_cast<Number*>(*it)->number();
			}

			return new Number(result);
		};

		auto mul = [](std::span<ASTNode*> nodes) -> ASTNode* {
			int64_t result = 1;

			for (auto node : nodes) {
				if (!is<Number>(node)) {
					Error::the().addError(format("wrong type argument: number-or-marker-p, '{}'", node));
					return nullptr;
				}

				result *= static_cast<Number*>(node)->number();
			}

			return new Number(result);
		};

		auto div = [this](std::span<ASTNode*> nodes) -> ASTNode* {
			double result = 0;

			if (nodes.size() == 0) {
				Error::the().addError(format("wrong number of arguments: {}, 0", m_current_key));
				return nullptr;
			}

			for (auto node : nodes) {
				if (!is<Number>(node)) {
					Error::the().addError(format("wrong type argument: number-or-marker-p, '{}'", node));
					return nullptr;
				}
			}

			// Start with the first number
			result += static_cast<Number*>(nodes[0])->number();

			// Skip the first node
			for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {
				result /= static_cast<Number*>(*it)->number();
			}

			return new Number((int64_t)result);
		};

		m_values.emplace("+", new Function(add));
		m_values.emplace("-", new Function(sub));
		m_values.emplace("*", new Function(mul));
		m_values.emplace("/", new Function(div));
	}
	virtual ~GlobalEnvironment() = default;
};

} // namespace blaze

// associative data structure that maps symbols (the keys) to values

// values = anything, including other symbols.
// an environment is like a hash table

// value can map to:
// list
// vector
// hash-map
// symbol
// number
// string
// function
