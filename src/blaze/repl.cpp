/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdlib> // std::exit
#include <string>
#include <string_view>
#include <vector>

#include "ruc/format/print.h"

#include "blaze/env/environment.h"
#include "blaze/error.h"
#include "blaze/eval.h"
#include "blaze/forward.h"
#include "blaze/lexer.h"
#include "blaze/printer.h"
#include "blaze/reader.h"
#include "blaze/readline.h"
#include "blaze/repl.h"
#include "blaze/settings.h"

namespace blaze {

Readline g_readline;
EnvironmentPtr g_outer_env;

auto Repl::init() -> void
{
	g_outer_env = Environment::create();
	Environment::loadFunctions();
	Environment::installFunctions(g_outer_env);
}

auto Repl::cleanup() -> void
{
	g_outer_env = nullptr;
}

auto Repl::readline(const std::string& prompt) -> ValuePtr
{
	std::string input;
	if (g_readline.get(input, g_readline.createPrompt(prompt))) {
		return makePtr<String>(input);
	}

	return makePtr<Constant>();
}

auto Repl::read(std::string_view input) -> ValuePtr
{
	Lexer lexer(input);
	lexer.tokenize();
	if (Settings::the().getEnvBool("*DUMP-LEXER*")) {
		lexer.dump();
	}

	Reader reader(std::move(lexer.tokens()));
	reader.read();
	if (Settings::the().getEnvBool("*DUMP-READER*")) {
		reader.dump();
	}

	return reader.node();
}

auto Repl::eval(ValuePtr ast, EnvironmentPtr env) -> ValuePtr
{
	if (env == nullptr) {
		env = g_outer_env;
	}

	Eval eval(ast, env);
	eval.eval();

	return eval.ast();
}

auto Repl::print(ValuePtr value) -> std::string
{
	Printer printer;

	return printer.print(value, true);
}

auto Repl::rep(std::string_view input, EnvironmentPtr env) -> std::string
{
	Error::the().clearErrors();
	Error::the().setInput(input);

	return print(eval(read(input), env));
}

auto Repl::makeArgv(EnvironmentPtr env, std::vector<std::string> arguments) -> void
{
	size_t count = arguments.size();
	auto nodes = ValueVector();
	for (size_t i = 1; i < count; ++i) {
		nodes.push_back(makePtr<String>(arguments[i]));
	}
	env->set("*ARGV*", makePtr<List>(nodes));
}

} // namespace blaze
