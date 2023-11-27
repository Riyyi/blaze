/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <string_view>

#include "ruc/singleton.h"

namespace blaze {

class Readline {
public:
	Readline() = default;
	Readline(bool pretty_print, std::string_view history_path);
	virtual ~Readline();

	std::string createPrompt(const std::string& prompt);

	bool get(std::string& output, const std::string& prompt);
	bool get(std::string& output);

private:
	bool m_pretty_print { false };
	char* m_history_path;
	std::string m_prompt;
};

} // namespace blaze
