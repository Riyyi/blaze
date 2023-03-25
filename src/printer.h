/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "ast.h"
#include <string>

namespace blaze {

// Serializer -> return to string
class Printer {
public:
	Printer();
	virtual ~Printer();

	std::string print(ASTNode* node);
	std::string printNoErrorCheck(ASTNode* node);

private:
	void init();
	void printImpl(ASTNode* node);
	void printError();

	bool m_first_node { true };
	bool m_previous_node_is_list { false };
	std::string m_print;
};

} // namespace blaze
