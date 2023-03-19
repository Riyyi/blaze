#include <csignal>  // std::signal
#include <cstdlib>  // std::exit
#include <iostream> // std::cin
#include <string>   // std::getline
#include <string_view>

#include "error.h"
#include "ruc/format/color.h"

#include "ast.h"
#include "lexer.h"
#include "printer.h"
#include "reader.h"

#define PRETTY_PRINT 0

#if 1
auto read(std::string_view input) -> blaze::ASTNode*
{
	blaze::Lexer lexer(input);
	lexer.tokenize();
	// lexer.dump();
	blaze::Reader reader(std::move(lexer.tokens()));
	reader.read();
	// reader.dump();

	return reader.node();
}

auto eval(blaze::ASTNode* node) -> blaze::ASTNode*
{
	return node;
}

auto print(blaze::ASTNode* node) -> void
{
	blaze::Printer printer(node);
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

auto main() -> int
{
	// Signal callbacks
	std::signal(SIGINT, cleanup);
	std::signal(SIGTERM, cleanup);

	while (true) {
	#if PRETTY_PRINT
		print(fg(ruc::format::TerminalColor::Blue), "user>");
		print(" ");
		print("\033[1m");
	#else
		print("user> ");
	#endif
		std::string line;
		std::getline(std::cin, line);
	#if PRETTY_PRINT
		print("\033[0m");
	#endif

		// Exit with Ctrl-D
		if (std::cin.eof() || std::cin.fail()) {
			break;
		}

		rep(line);
	}

	return 0;
}
#endif
