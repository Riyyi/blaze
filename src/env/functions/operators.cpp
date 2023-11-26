/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdint> // int64_t
#include <memory>  // std::static_pointer_cast

#include "ast.h"
#include "env/macro.h"
#include "util.h"

namespace blaze {

void Environment::loadOperators()
{
#define APPLY_NUMBER_OR_DECIMAL(it, apply)                                   \
	IS_VALUE(Numeric, (*it));                                                \
	if (is<Number>(it->get())) {                                             \
		auto it_numeric = std::static_pointer_cast<Number>(*it)->number();   \
		do {                                                                 \
			apply                                                            \
		} while (0);                                                         \
	}                                                                        \
	else {                                                                   \
		return_decimal = true;                                               \
		auto it_numeric = std::static_pointer_cast<Decimal>(*it)->decimal(); \
		do {                                                                 \
			apply                                                            \
		} while (0);                                                         \
	}

#define RETURN_NUMBER_OR_DECIMAL()      \
	if (!return_decimal) {              \
		return makePtr<Number>(number); \
	}                                   \
	return makePtr<Decimal>(decimal);

	ADD_FUNCTION(
		"+",
		"number...",
		"Return the sum of any amount of arguments, where NUMBER is of type number.",
		{
			bool return_decimal = false;

			int64_t number = 0;
			double decimal = 0;

			for (auto it = begin; it != end; ++it) {
				APPLY_NUMBER_OR_DECIMAL(it, {
					number += it_numeric;
					decimal += it_numeric;
				});
			}

			RETURN_NUMBER_OR_DECIMAL();
		});

	ADD_FUNCTION(
		"-",
		"[number] subtract...",
		R"(Negate NUMBER or SUBTRACT numbers and return the result.

With one arg, negates it. With more than one arg,
subtracts all but the first from the first.)",
		{
			size_t length = SIZE();
			if (length == 0) {
				return makePtr<Number>(0);
			}

			bool return_decimal = false;

			int64_t number = 0;
			double decimal = 0;

			// Start with the first number
			APPLY_NUMBER_OR_DECIMAL(begin, {
				number = it_numeric;
				decimal = it_numeric;
			});

			// Return negative on single argument
			if (length == 1) {
				number = -number;
				decimal = -decimal;
				RETURN_NUMBER_OR_DECIMAL();
			}

			// Skip the first node
			for (auto it = begin + 1; it != end; ++it) {
				APPLY_NUMBER_OR_DECIMAL(it, {
					number -= it_numeric;
					decimal -= it_numeric;
				});
			}

			RETURN_NUMBER_OR_DECIMAL();
		});

	ADD_FUNCTION(
		"*",
		"",
		"",
		{
			bool return_decimal = false;

			int64_t number = 1;
			double decimal = 1;

			for (auto it = begin; it != end; ++it) {
				APPLY_NUMBER_OR_DECIMAL(it, {
					number *= it_numeric;
					decimal *= it_numeric;
				});
			}

			RETURN_NUMBER_OR_DECIMAL();
		});

	ADD_FUNCTION(
		"/",
		"",
		"",
		{
			CHECK_ARG_COUNT_AT_LEAST("/", SIZE(), 1);

			bool return_decimal = false;

			int64_t number = 0;
			double decimal = 0;

			// Start with the first number
			APPLY_NUMBER_OR_DECIMAL(begin, {
				number = it_numeric;
				decimal = it_numeric;
			});

			// Skip the first node
			for (auto it = begin + 1; it != end; ++it) {
				APPLY_NUMBER_OR_DECIMAL(it, {
					number /= it_numeric;
					decimal /= it_numeric;
				});
			}

			RETURN_NUMBER_OR_DECIMAL();
		});

	// (% 5 2) -> 1
	ADD_FUNCTION(
		"%",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("/", SIZE(), 2);

			VALUE_CAST(divide, Number, (*begin));
			VALUE_CAST(by, Number, (*(begin + 1)));

			return makePtr<Number>(divide->number() % by->number());
		});
}

} // namespace blaze
