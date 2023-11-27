/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "blaze/forward.h"
#include "blaze/readline.h"

namespace blaze {

class Repl {
public:
	static auto cleanup(int signal) -> void;
	static auto eval(ValuePtr ast, EnvironmentPtr env) -> ValuePtr;
	static auto makeArgv(EnvironmentPtr env, std::vector<std::string> arguments) -> void;
	static auto print(ValuePtr value) -> std::string;
	static auto read(std::string_view input) -> ValuePtr;
	static auto readline(const std::string& prompt) -> ValuePtr;
	static auto rep(std::string_view input, EnvironmentPtr env) -> std::string;
};

} // namespace blaze
