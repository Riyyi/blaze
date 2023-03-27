/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string>
#include <string_view>

namespace blaze {

template<typename It, typename C>
inline bool isLast(It it, const C& container)
{
	return (it != container.end()) && (next(it) == container.end());
}

inline std::string replaceAll(std::string text, std::string_view search, std::string_view replace)
{
	size_t search_length = search.length();
	size_t replace_length = replace.length();
	size_t position = text.find(search, 0);
	while (position != std::string::npos) {
		text.replace(position, search_length, replace);
		position += replace_length;
		position = text.find(search, position);
	}

	return text;
}

} // namespace blaze
