#include <cstdio>
#include <iostream> // std::cin
#include <string>   // std::getline
#include <string_view>

#include "ast.h"
#include "lexer.h"
#include "printer.h"
#include "reader.h"

#if 1
auto read(std::string_view data) -> blaze::ASTNode*
{
	blaze::Lexer lexer(data);
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

auto rep(std::string_view data) -> void
{
	print(eval(read(data)));
}

auto main() -> int
{
	while (true) {
		printf("user> ");
		std::string line;
		std::getline(std::cin, line);

		// Exit with Ctrl-D
		if (std::cin.eof() || std::cin.fail()) {
			break;
		}

		rep(line);
	}

	return 0;
}
#endif
