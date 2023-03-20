/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "ruc/meta/assert.h"
#include "ruc/singleton.h"
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace blaze {

class Settings final : public ruc::Singleton<Settings> {
public:
	Settings(s) {}
	virtual ~Settings() = default;

	std::string_view get(std::string_view key) const;
	void set(std::string_view key, std::string_view value) { m_settings[key] = value; };

private:
	std::unordered_map<std::string_view, std::string_view> m_settings;
};

} // namespace blaze
