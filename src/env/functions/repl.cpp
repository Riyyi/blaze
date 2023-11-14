/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <string>

#include "ruc/file.h"

#include "ast.h"
#include "env/macro.h"
#include "util.h"

namespace blaze {

void Environment::loadRepl()
{
	// REPL reader
	ADD_FUNCTION(
		"read-string",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("read-string", SIZE(), 1);

			VALUE_CAST(node, String, (*begin));
			std::string input = node->data();

			return read(input);
		});

	// Read file contents
	ADD_FUNCTION(
		"slurp",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("slurp", SIZE(), 1);

			VALUE_CAST(node, String, (*begin));
			std::string path = node->data();

			auto file = ruc::File(path);

			return makePtr<String>(file.data());
		});

	// Prompt readline
	ADD_FUNCTION(
		"readline",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("readline", SIZE(), 1);

			VALUE_CAST(prompt, String, (*begin));

			return readline(prompt->data());
		});

	// REPL eval
	ADD_FUNCTION(
		"eval",
		"",
		"",
		{
			CHECK_ARG_COUNT_IS("eval", SIZE(), 1);

			return eval(*begin, nullptr);
		});
}

} // namespace blaze
