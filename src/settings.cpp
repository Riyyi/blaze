/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#include <memory> // std::static_pointer_cast

#include "ruc/meta/assert.h"

#include "env/environment.h"
#include "forward.h"
#include "settings.h"
#include "types.h"

namespace blaze {

std::string_view Settings::get(std::string_view key) const
{
	VERIFY(m_settings.find(key) != m_settings.end(), "setting does not exist");
	return m_settings.at(key);
};

bool Settings::getEnvBool(std::string_view key) const
{
	auto env = g_outer_env->get(key);
	return is<Constant>(env.get()) && std::static_pointer_cast<Constant>(env)->state() == Constant::State::True;
}

} // namespace blaze
