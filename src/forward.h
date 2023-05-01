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
typedef std::shared_ptr<Value> ValuePtr;
typedef std::vector<ValuePtr> ValueVector;
typedef ValueVector::iterator ValueVectorIt;
typedef ValueVector::const_iterator ValueVectorConstIt;

class Environment;
typedef std::shared_ptr<Environment> EnvironmentPtr;

// -----------------------------------------
// Functions

extern ValuePtr readline(const std::string& prompt);
extern ValuePtr read(std::string_view input);
extern ValuePtr eval(ValuePtr ast, EnvironmentPtr env);

extern void installFunctions(EnvironmentPtr env);

} // namespace blaze
