/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdint>    // int64_t
#include <functional> // std::function
#include <memory>     // std::static_pointer_cast

#include "ast.h"
#include "env/macro.h"
#include "util.h"

namespace blaze {

void Environment::loadCompare()
{
#define NUMBER_COMPARE(operator)                                               \
	{                                                                          \
		bool result = true;                                                    \
                                                                               \
		CHECK_ARG_COUNT_AT_LEAST(#operator, SIZE(), 2);                        \
                                                                               \
		/* Start with the first number */                                      \
		VALUE_CAST(number_node, Number, (*begin));                             \
		int64_t number = number_node->number();                                \
                                                                               \
		/* Skip the first node */                                              \
		for (auto it = begin + 1; it != end; ++it) {                           \
			VALUE_CAST(current_number_node, Number, (*it));                    \
			int64_t current_number = current_number_node->number();            \
			if (!(number operator current_number)) {                           \
				result = false;                                                \
				break;                                                         \
			}                                                                  \
			number = current_number;                                           \
		}                                                                      \
                                                                               \
		return makePtr<Constant>((result) ? Constant::True : Constant::False); \
	}

	ADD_FUNCTION("<", NUMBER_COMPARE(<));
	ADD_FUNCTION("<=", NUMBER_COMPARE(<=));
	ADD_FUNCTION(">", NUMBER_COMPARE(>));
	ADD_FUNCTION(">=", NUMBER_COMPARE(>=));

	// -----------------------------------------

	// (= 1 1)         -> true
	// (= "foo" "foo") -> true
	ADD_FUNCTION(
		"=",
		{
			CHECK_ARG_COUNT_AT_LEAST("=", SIZE(), 2);

			std::function<bool(ValuePtr, ValuePtr)> equal =
				[&equal](ValuePtr lhs, ValuePtr rhs) -> bool {
				if ((is<List>(lhs.get()) || is<Vector>(lhs.get()))
			        && (is<List>(rhs.get()) || is<Vector>(rhs.get()))) {
					auto lhs_collection = std::static_pointer_cast<Collection>(lhs);
					auto rhs_collection = std::static_pointer_cast<Collection>(rhs);

					if (lhs_collection->size() != rhs_collection->size()) {
						return false;
					}

					auto lhs_it = lhs_collection->begin();
					auto rhs_it = rhs_collection->begin();
					for (; lhs_it != lhs_collection->end(); ++lhs_it, ++rhs_it) {
						if (!equal(*lhs_it, *rhs_it)) {
							return false;
						}
					}

					return true;
				}

				if (is<HashMap>(lhs.get()) && is<HashMap>(rhs.get())) {
					const auto& lhs_nodes = std::static_pointer_cast<HashMap>(lhs)->elements();
					const auto& rhs_nodes = std::static_pointer_cast<HashMap>(rhs)->elements();

					if (lhs_nodes.size() != rhs_nodes.size()) {
						return false;
					}

					for (const auto& [key, value] : lhs_nodes) {
						auto it = rhs_nodes.find(key);
						if (it == rhs_nodes.cend() || !equal(value, it->second)) {
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
				if (is<Constant>(lhs.get()) && is<Constant>(rhs.get())
			        && std::static_pointer_cast<Constant>(lhs)->state() == std::static_pointer_cast<Constant>(rhs)->state()) {
					return true;
				}
				if (is<Symbol>(lhs.get()) && is<Symbol>(rhs.get())
			        && std::static_pointer_cast<Symbol>(lhs)->symbol() == std::static_pointer_cast<Symbol>(rhs)->symbol()) {
					return true;
				}

				return false;
			};

			bool result = true;
			auto it = begin;
			auto it_next = begin + 1;
			for (; it_next != end; ++it, ++it_next) {
				if (!equal(*it, *it_next)) {
					result = false;
					break;
				}
			}

			return makePtr<Constant>((result) ? Constant::True : Constant::False);
		});
}

} // namespace blaze
