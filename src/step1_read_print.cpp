#include <csignal>  // std::signal
#include <cstdlib>  // std::exit
#include <iostream> // std::cin
#include <string>   // std::getline
#include <string_view>

#include "ruc/argparser.h"
#include "ruc/format/color.h"

#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "printer.h"
#include "reader.h"
#include "settings.h"

#if 1
auto read(std::string_view input) -> blaze::ASTNode*
{
	blaze::Lexer lexer(input);
	lexer.tokenize();
	if (blaze::Settings::the().get("dump-lexer") == "1") {
		lexer.dump();
	}

	blaze::Reader reader(std::move(lexer.tokens()));
	reader.read();
	if (blaze::Settings::the().get("dump-reader") == "1") {
		reader.dump();
	}

	return reader.node();
}

auto eval(blaze::ASTNode* ast) -> blaze::ASTNode*
{
	return ast;
}

auto print(blaze::ASTNode* exp) -> void
{
	blaze::Printer printer(exp);
	printer.dump();
}

auto rep(std::string_view input) -> void
{
	blaze::Error::the().clearErrors();
	blaze::Error::the().setInput(input);

	print(eval(read(input)));
}

static auto cleanup(int signal) -> void
{
	print("\033[0m");
	std::exit(signal);
}

auto main(int argc, char* argv[]) -> int
{
	bool dump_lexer = false;
	bool dump_reader = false;
	bool pretty_print = false;

	// CLI arguments
	ruc::ArgParser arg_parser;
	arg_parser.addOption(dump_lexer, 'l', "dump-lexer", nullptr, nullptr);
	arg_parser.addOption(dump_reader, 'r', "dump-reader", nullptr, nullptr);
	arg_parser.addOption(pretty_print, 'c', "color", nullptr, nullptr);
	arg_parser.parse(argc, argv);

	// Set settings
	blaze::Settings::the().set("dump-lexer", dump_lexer ? "1" : "0");
	blaze::Settings::the().set("dump-reader", dump_reader ? "1" : "0");
	blaze::Settings::the().set("pretty-print", pretty_print ? "1" : "0");

	// Signal callbacks
	std::signal(SIGINT, cleanup);
	std::signal(SIGTERM, cleanup);

	while (true) {
		if (!pretty_print) {
			print("user> ");
		}
		else {
			print(fg(ruc::format::TerminalColor::Blue), "user>");
			print(" \033[1m");
		}
		std::string line;
		std::getline(std::cin, line);
		if (pretty_print) {
			print("\033[0m");
		}

		// Exit with Ctrl-D
		if (std::cin.eof() || std::cin.fail()) {
			break;
		}

		rep(line);
	}

	return 0;
}
#endif
