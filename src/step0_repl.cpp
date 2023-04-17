#include <cstdio>
#include <iostream> // std::cin
#include <string>   // std::getline
#include <string_view>

#include "forward.h"

#if 0
auto read(std::string_view data) -> std::string_view
{
	return data;
}

auto eval(std::string_view data) -> std::string_view
{
	return data;
}

auto print(std::string_view data) -> void
{
	printf("%s\n", data.data());
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

// Below is needed for compilation
namespace blaze {

auto read(std::string_view) -> ValuePtr
{
	return {};
}

auto eval(ValuePtr, EnvironmentPtr) -> ValuePtr
{
	return {};
}

} // namespace blaze
#endif
