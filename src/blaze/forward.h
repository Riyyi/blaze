/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory> // std::shared_ptr
#include <vector>

namespace blaze {

// -----------------------------------------
// Types

class Value;
class HashMap;
typedef std::shared_ptr<Value> ValuePtr;
typedef std::shared_ptr<HashMap> HashMapPtr;
typedef std::vector<ValuePtr> ValueVector;
typedef ValueVector::iterator ValueVectorIt;
typedef ValueVector::reverse_iterator ValueVectorReverseIt;
typedef ValueVector::const_iterator ValueVectorConstIt;
typedef ValueVector::const_reverse_iterator ValueVectorConstReverseIt;

class Environment;
typedef std::shared_ptr<Environment> EnvironmentPtr;

class Readline;

// -----------------------------------------
// Functions

// -----------------------------------------
// Variables

extern Readline g_readline;
extern EnvironmentPtr g_outer_env;

} // namespace blaze
