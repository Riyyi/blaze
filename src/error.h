/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <vector>

#include "ruc/singleton.h"

#include "forward.h"
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
		m_exceptions.clear();
	}
	void add(Token error) { m_token_errors.push_back(error); }
	void add(const std::string& error) { m_other_errors.push_back(error); }
	void add(ValuePtr error) { m_exceptions.push_back(error); }

	bool hasTokenError() { return m_token_errors.size() > 0; }
	bool hasOtherError() { return m_other_errors.size() > 0; }
	bool hasException() { return m_exceptions.size() > 0; }
	bool hasAnyError() { return hasTokenError() || hasOtherError() || hasException(); }

	void setInput(std::string_view input) { m_input = input; }

	Token tokenError() const { return m_token_errors[0]; }
	const std::string& otherError() const { return m_other_errors[0]; }
	ValuePtr exception() const { return m_exceptions[0]; }

private:
	std::string_view m_input;
	std::vector<Token> m_token_errors;
	std::vector<std::string> m_other_errors;
	std::vector<ValuePtr> m_exceptions;
};

} // namespace blaze
