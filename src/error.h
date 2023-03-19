/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <vector>

#include "ruc/singleton.h"

#include "lexer.h"

namespace blaze {

class Error final : public ruc::Singleton<Error> {
public:
	Error(s) {}
	virtual ~Error() = default;

	void clearErrors()
	{
		m_token_errors.clear();
		m_other_errors.clear();
	}
	void addError(Token error) { m_token_errors.push_back(error); }
	void addError(const std::string& error) { m_other_errors.push_back(error); }

	bool hasTokenError() { return m_token_errors.size() > 0; }
	bool hasOtherError() { return m_other_errors.size() > 0; }
	bool hasAnyError() { return hasTokenError() || hasOtherError(); }

	void setInput(std::string_view input) { m_input = input; }

	Token tokenError() const { return m_token_errors[0]; }
	const std::string& otherError() const { return m_other_errors[0]; }

private:
	std::string_view m_input;
	std::vector<Token> m_token_errors;
	std::vector<std::string> m_other_errors;
};

} // namespace blaze
