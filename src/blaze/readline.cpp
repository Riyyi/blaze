/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <cstdlib>  // std::free
#include <iostream> // FILE
#include <readline/history.h>
#include <readline/readline.h>
#include <readline/tilde.h>
#include <string>
#include <string_view>

#include "ruc/format/color.h"
#include "ruc/format/print.h"

#include "blaze/readline.h"

namespace blaze {

Readline::Readline(bool pretty_print, std::string_view history_path)
	: m_pretty_print(pretty_print)
	, m_history_path(tilde_expand(history_path.data()))
{
	m_prompt = createPrompt("user> ");

	read_history(m_history_path);
}

Readline::~Readline()
{
	std::free(m_history_path);
}

// -----------------------------------------

std::string Readline::createPrompt(const std::string& prompt)
{
	if (!m_pretty_print) {
		return prompt;
	}

	return format(fg(ruc::format::TerminalColor::Blue), "{}", prompt)
	       + format("\033[1m");
}

bool Readline::get(std::string& output, const std::string& prompt)
{
	char* line = readline(prompt.c_str());

	if (m_pretty_print) {
		print("\033[0m");
	}

	if (line == nullptr) {
		return false;
	}

	// Add input to in-memory history
	add_history(line);
	append_history(1, m_history_path);

	output = line;
	std::free(line);

	return true;
}

bool Readline::get(std::string& output)
{
	return get(output, m_prompt);
}

} // namespace blaze
