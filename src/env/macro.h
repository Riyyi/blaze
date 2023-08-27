/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <unordered_map>

#include "env/environment.h"

#define ADD_FUNCTION(symbol, lambda) \
	Environment::registerFunction(   \
		symbol,                      \
		[](ValueVectorConstIt begin, ValueVectorConstIt end) -> blaze::ValuePtr lambda);

#define SIZE() std::distance(begin, end)
