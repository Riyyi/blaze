/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <csignal> // std::signal
#include <string>
#include <string_view>
#include <vector>

#include "ruc/argparser.h"
#include "ruc/format/color.h"
#include "ruc/format/print.h"

#include "blaze/ast.h"
#include "blaze/env/environment.h"
#include "blaze/forward.h"
#include "blaze/repl.h"
#include "blaze/settings.h"

namespace blaze {

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
	g_outer_env->set("*DUMP-LEXER*", makePtr<Constant>(dump_lexer));
	g_outer_env->set("*DUMP-READER*", makePtr<Constant>(dump_reader));
	g_outer_env->set("*PRETTY-PRINT*", makePtr<Constant>(pretty_print));

	// Signal callbacks
	std::signal(SIGINT, Repl::cleanup);
	std::signal(SIGTERM, Repl::cleanup);

	Environment::loadFunctions();
	Environment::installFunctions(g_outer_env);
	Repl::makeArgv(g_outer_env, arguments);

	if (arguments.size() > 0) {
		Repl::rep(format("(load-file \"{}\")", arguments.front()), g_outer_env);
		return 0;
	}

	Repl::rep("(println (str \"Blaze [\" *host-language* \"]\"))", g_outer_env);

	g_readline = Readline(pretty_print, history_path);

	std::string input;
	while (g_readline.get(input)) {
		std::string output = Repl::rep(input, g_outer_env);
		if (output.length() > 0) {
			print("{}\n", output);
		}
	}

	if (pretty_print) {
		print("\033[0m");
	}

	return 0;
}

} // namespace blaze

auto main(int argc, char* argv[]) -> int
{
	return blaze::main(argc, argv);
}
