/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include "settings.h"

namespace blaze {

std::string_view Settings::get(std::string_view key) const
{
	VERIFY(m_settings.find(key) != m_settings.end(), "setting does not exist");
	return m_settings.at(key);
};

} // namespace blaze
