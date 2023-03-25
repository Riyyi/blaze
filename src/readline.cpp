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

#include "ruc/format/color.h"

#include "readline.h"

namespace blaze {

Readline::Readline(bool pretty_print, std::string_view history_path)
	: m_pretty_print(pretty_print)
	, m_history_path(history_path)
{
	if (!pretty_print) {
		m_prompt = "user> ";
	}
	else {
		m_prompt = format(fg(ruc::format::TerminalColor::Blue), "user>");
		m_prompt += format(" \033[1m");
	}

	read_history(tilde_expand(history_path.data()));
}

bool Readline::get(std::string& output)
{
	char* line = readline(m_prompt.c_str());
	if (line == nullptr) {
		return false;
	}

	// Add input to in-memory history
	add_history(line);
	append_history(1, m_history_path.data());

	output = line;
	std::free(line);

	return true;
}

} // namespace blaze