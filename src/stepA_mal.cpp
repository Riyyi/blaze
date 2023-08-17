/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <csignal> // std::signal
#include <cstdlib> // std::exit
#include <string>
#include <string_view>
#include <vector>

#include "ruc/argparser.h"
#include "ruc/format/color.h"

#include "ast.h"
#include "environment.h"
#include "error.h"
#include "eval.h"
#include "forward.h"
#include "lexer.h"
#include "printer.h"
#include "reader.h"
#include "readline.h"
#include "settings.h"

namespace blaze {

static blaze::Readline s_readline;
static EnvironmentPtr s_outer_env = Environment::create();

static auto cleanup(int signal) -> void
{
	print("\033[0m\n");
	std::exit(signal);
}

auto readline(const std::string& prompt) -> ValuePtr
{
	std::string input;
	if (s_readline.get(input, s_readline.createPrompt(prompt))) {
		return makePtr<String>(input);
	}

	return makePtr<Constant>();
}

auto read(std::string_view input) -> ValuePtr
{
	Lexer lexer(input);
	lexer.tokenize();
	if (Settings::the().get("dump-lexer") == "1") {
		lexer.dump();
	}

	Reader reader(std::move(lexer.tokens()));
	reader.read();
	if (Settings::the().get("dump-reader") == "1") {
		reader.dump();
	}

	return reader.node();
}

auto eval(ValuePtr ast, EnvironmentPtr env) -> ValuePtr
{
	if (env == nullptr) {
		env = s_outer_env;
	}

	Eval eval(ast, env);
	eval.eval();

	return eval.ast();
}

static auto print(ValuePtr exp) -> std::string
{
	Printer printer;

	return printer.print(exp, true);
}

static auto rep(std::string_view input, EnvironmentPtr env) -> std::string
{
	Error::the().clearErrors();
	Error::the().setInput(input);

	return print(eval(read(input), env));
}

static std::string_view lambdaTable[] = {
	"(def! not (fn* (cond) (if cond false true)))",
	"(def! load-file (fn* (filename) \
	   (eval (read-string (str \"(do \" (slurp filename) \"\nnil)\")))))",
	"(defmacro! cond (fn* (& xs) \
	    (if (> (count xs) 0) \
	        (list 'if (first xs) \
	            (if (> (count xs) 1) \
	                (nth xs 1) \
	              (throw \"odd number of forms to cond\")) \
	          (cons 'cond (rest (rest xs)))))))",
	"(def! *host-language* \"C++\")",
};

static auto installLambdas(EnvironmentPtr env) -> void
{
	for (auto function : lambdaTable) {
		rep(function, env);
	}
}

static auto makeArgv(EnvironmentPtr env, std::vector<std::string> arguments) -> void
{
	size_t count = arguments.size();
	auto nodes = ValueVector();
	for (size_t i = 1; i < count; ++i) {
		nodes.push_back(makePtr<String>(arguments[i]));
	}
	env->set("*ARGV*", makePtr<List>(nodes));
}

} // namespace blaze

auto main(int argc, char* argv[]) -> int
{
	bool dump_lexer = false;
	bool dump_reader = false;
	bool pretty_print = false;
	std::string_view history_path = "~/.blaze-history";
	std::vector<std::string> arguments;

	// CLI arguments
	ruc::ArgParser arg_parser;
	arg_parser.addOption(dump_lexer, 'l', "dump-lexer", nullptr, nullptr);
	arg_parser.addOption(dump_reader, 'r', "dump-reader", nullptr, nullptr);
	arg_parser.addOption(pretty_print, 'c', "color", nullptr, nullptr);
	arg_parser.addOption(history_path, 'h', "history-path", nullptr, nullptr, nullptr, ruc::ArgParser::Required::Yes);
	// TODO: Add overload for addArgument(std::vector<std::string_view>)
	arg_parser.addArgument(arguments, "arguments", nullptr, nullptr, ruc::ArgParser::Required::No);
	arg_parser.parse(argc, argv);

	// Set settings
	blaze::Settings::the().set("dump-lexer", dump_lexer ? "1" : "0");
	blaze::Settings::the().set("dump-reader", dump_reader ? "1" : "0");
	blaze::Settings::the().set("pretty-print", pretty_print ? "1" : "0");

	// Signal callbacks
	std::signal(SIGINT, blaze::cleanup);
	std::signal(SIGTERM, blaze::cleanup);

	installFunctions(blaze::s_outer_env);
	installLambdas(blaze::s_outer_env);
	makeArgv(blaze::s_outer_env, arguments);

	if (arguments.size() > 0) {
		rep(format("(load-file \"{}\")", arguments.front()), blaze::s_outer_env);
		return 0;
	}

	rep("(println (str \"Mal [\" *host-language* \"]\"))", blaze::s_outer_env);

	blaze::s_readline = blaze::Readline(pretty_print, history_path);

	std::string input;
	while (blaze::s_readline.get(input)) {
		std::string output = rep(input, blaze::s_outer_env);
		if (output.length() > 0) {
			print("{}\n", output);
		}
	}

	if (pretty_print) {
		print("\033[0m");
	}

	return 0;
}

// TODO: List of things below
// v Pass stepA tests (implement leftover functions)
// v Make Macro into a subclass of Lambda (?) to simplify some logic
// v Convert std::list -> std::vector, test speed before/after
// v Better way of running mal tests
// - Make Collections/HashMaps const, construct a new one with the added items
// - Basic performance testing macros, enabled in debug builds

// Iterations to beat:
//   Debug:   ~5k
//   Release: ~30k
// New:
//   Debug:   ~6k
//   Release: ~37k
