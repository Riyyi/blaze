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
		int64_t result = 0;

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
		result += std::static_pointer_cast<Number>(nodes[0])->number();

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
		double result = 0;

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
		result += std::static_pointer_cast<Number>(nodes[0])->number();

		// Skip the first node
		for (auto it = std::next(nodes.begin()); it != nodes.end(); ++it) {
			result /= std::static_pointer_cast<Number>(*it)->number();
		}

		return makePtr<Number>((int64_t)result);
	};

	m_values.emplace("/", makePtr<Function>(div));
}

} // namespace blaze
