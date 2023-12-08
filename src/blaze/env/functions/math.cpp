/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cmath>  // std::sin
#include <limits> // sdt::numeric_limits
#include <memory> // std::static_pointer_cast

#include "blaze/ast.h"
#include "blaze/env/macro.h"
#include "blaze/util.h"

namespace blaze {

void Environment::loadMath()
{
#define MATH_MAX_MIN(variant, limit, operator)                                       \
	{                                                                                \
		CHECK_ARG_COUNT_AT_LEAST(#variant, SIZE(), 1);                               \
                                                                                     \
		int64_t number = std::numeric_limits<int64_t>::limit();                      \
		double decimal = std::numeric_limits<double>::limit();                       \
                                                                                     \
		for (auto it = begin; it != end; ++it) {                                     \
			IS_VALUE(Numeric, (*it));                                                \
			if (is<Number>(it->get())) {                                             \
				auto it_numeric = std::static_pointer_cast<Number>(*it)->number();   \
				if (it_numeric operator number) {                                    \
					number = it_numeric;                                             \
				}                                                                    \
			}                                                                        \
			else {                                                                   \
				auto it_numeric = std::static_pointer_cast<Decimal>(*it)->decimal(); \
				if (it_numeric operator decimal) {                                   \
					decimal = it_numeric;                                            \
				}                                                                    \
			}                                                                        \
		}                                                                            \
                                                                                     \
		if (number operator decimal) {                                               \
			return makePtr<Number>(number);                                          \
		}                                                                            \
                                                                                     \
		return makePtr<Decimal>(decimal);                                            \
	}

	ADD_FUNCTION(
		"max", "number...",
		"Return largest of all arguments, where NUMBER is a number or decimal.",
		MATH_MAX_MIN(max, lowest, >));
	ADD_FUNCTION(
		"min", "number...",
		"Return smallest of all arguments, where NUMBER is a number or decimal.",
		MATH_MAX_MIN(min, max, <));

#define MATH_COS_SIN(variant)                                                                                 \
	{                                                                                                         \
		CHECK_ARG_COUNT_IS(#variant, SIZE(), 1);                                                              \
                                                                                                              \
		auto value = *begin;                                                                                  \
		IS_VALUE(Numeric, value);                                                                             \
		if (is<Number>(begin->get())) {                                                                       \
			return makePtr<Decimal>(std::variant((double)std::static_pointer_cast<Number>(value)->number())); \
		}                                                                                                     \
                                                                                                              \
		return makePtr<Decimal>(std::variant(std::static_pointer_cast<Decimal>(value)->decimal()));           \
	}

	ADD_FUNCTION(
		"cos", "arg",
		"Return the cosine of ARG.",
		MATH_COS_SIN(cos));

	ADD_FUNCTION(
		"sin", "arg",
		"Return the sine of ARG.",
		MATH_COS_SIN(sin));
}

} // namespace blaze
