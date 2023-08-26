/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <unordered_map>

#include "env/environment.h"

// At the top-level you cant invoke any function, but you can create variables.
// Using a struct's constructor you can work around this limitation.
// Also, the counter macro is used to make the struct names unique.

#define FUNCTION_STRUCT_NAME(unique) __functionStruct##unique

#define ADD_FUNCTION_IMPL(unique, symbol, lambda)              \
	struct FUNCTION_STRUCT_NAME(unique) {                      \
		FUNCTION_STRUCT_NAME(unique)                           \
		(const std::string& __symbol, FunctionType __lambda)   \
		{                                                      \
			Environment::registerFunction(__symbol, __lambda); \
		}                                                      \
	};                                                         \
	static struct FUNCTION_STRUCT_NAME(unique)                 \
		FUNCTION_STRUCT_NAME(unique)(                          \
			symbol,                                            \
			[](ValueVectorConstIt begin, ValueVectorConstIt end) -> ValuePtr lambda);

#define ADD_FUNCTION(symbol, lambda) ADD_FUNCTION_IMPL(__COUNTER__, symbol, lambda);

#define SIZE() std::distance(begin, end)
