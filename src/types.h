/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <typeinfo>

template<typename T, typename U>
inline bool is(U& input)
{
	if constexpr (requires { input.template fastIs<T>(); }) {
		return input.template fastIs<T>();
	}

	return typeid(input) == typeid(T);
}

template<typename T, typename U>
inline bool is(U* input)
{
	return input && is<T>(*input);
}

// serenity/AK/TypeCasts.h
// serenity/Userland/Libraries/LibJS/AST.h
