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
		{
			if (SIZE() == 0) {
				return makePtr<Number>(0);
			}

			// Start with the first number
			VALUE_CAST(number, Number, (*begin));
			int64_t result = number->number();

			// Skip the first node
			for (auto it = begin + 1; it != end; ++it) {
				VALUE_CAST(number, Number, (*it));
				result -= number->number();
			}

			return makePtr<Number>(result);
		});

	ADD_FUNCTION(
		"*",
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
		{
			CHECK_ARG_COUNT_IS("/", SIZE(), 2);

			VALUE_CAST(divide, Number, (*begin));
			VALUE_CAST(by, Number, (*(begin + 1)));

			return makePtr<Number>(divide->number() % by->number());
		});
}

} // namespace blaze
