/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <unordered_map>

#include "blaze/env/environment.h"

#define ADD_FUNCTION(name, signature, documentation, lambda) \
	Environment::registerFunction(                           \
		{ name,                                              \
	      signature,                                         \
	      documentation,                                     \
	      [](ValueVectorConstIt begin, ValueVectorConstIt end) -> blaze::ValuePtr lambda });

#define SIZE() std::distance(begin, end)
