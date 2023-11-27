/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <string_view>
#include <unordered_map>

#include "ruc/singleton.h"

namespace blaze {

class Settings final : public ruc::Singleton<Settings> {
public:
	Settings(s) {}
	virtual ~Settings() = default;

	std::string_view get(std::string_view key) const;
	void set(std::string_view key, std::string_view value) { m_settings[key] = value; };

	bool getEnvBool(std::string_view key) const;

private:
	std::unordered_map<std::string_view, std::string_view> m_settings;
};

} // namespace blaze
