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
	Readline(bool pretty_print, std::string_view history_path);
	virtual ~Readline();

	bool get(std::string& output);

private:
	bool m_pretty_print { false };
	std::string m_prompt;
	char* m_history_path;
};

} // namespace blaze
