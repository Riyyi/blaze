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

#include "env/environment.h"
#include "error.h"
#include "eval.h"
#include "forward.h"
#include "lexer.h"
#include "printer.h"
#include "reader.h"
#include "readline.h"
#include "repl.h"
#include "settings.h"

namespace blaze {

Readline g_readline;
EnvironmentPtr g_outer_env = Environment::create();

auto Repl::cleanup(int signal) -> void
{
	::print("\033[0m\n");
	std::exit(signal);
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
