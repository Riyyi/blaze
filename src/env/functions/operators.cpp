/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdint> // int64_t

#include "ast.h"
#include "env/macro.h"
#include "util.h"

namespace blaze {

void Environment::loadOperators()
{
	ADD_FUNCTION(
		"+",
		"number...",
		"Return the sum of any amount of arguments, where NUMBER is of type number.",
		{
			int64_t result = 0;

			for (auto it = begin; it != end; ++it) {
				VALUE_CAST(number, Number, (*it));
				result += number->number();
			}

			return makePtr<Number>(result);
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

			// Start with the first number
			VALUE_CAST(number, Number, (*begin));
			int64_t result = number->number();

			if (length == 1) {
				return makePtr<Number>(-result);
			}

			// Skip the first node
			for (auto it = begin + 1; it != end; ++it) {
				VALUE_CAST(number, Number, (*it));
				result -= number->number();
			}

			return makePtr<Number>(result);
		});

	ADD_FUNCTION(
		"*",
		"",
		"",
		{
			int64_t result = 1;

			for (auto it = begin; it != end; ++it) {
				VALUE_CAST(number, Number, (*it));
				result *= number->number();
			}

			return makePtr<Number>(result);
		});

	ADD_FUNCTION(
		"/",
		"",
		"",
		{
			CHECK_ARG_COUNT_AT_LEAST("/", SIZE(), 1);

			// Start with the first number
			VALUE_CAST(number, Number, (*begin));
			double result = number->number();

			// Skip the first node
			for (auto it = begin + 1; it != end; ++it) {
				VALUE_CAST(number, Number, (*it));
				result /= number->number();
			}

			return makePtr<Number>((int64_t)result);
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
