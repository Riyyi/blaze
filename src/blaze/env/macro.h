/*
 * Copyright (C) 2023 Riyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <iterator> // std::distance
#include <unordered_map>

#include "blaze/env/environment.h"
#include "blaze/forward.h"

#define ADD_FUNCTION(name, signature, documentation, lambda) \
	blaze::Environment::registerFunction(                    \
		{ name,                                              \
	      signature,                                         \
	      documentation,                                     \
	      [](blaze::ValueVectorConstIt begin, blaze::ValueVectorConstIt end) -> blaze::ValuePtr lambda });

#define SIZE() std::distance(begin, end)
