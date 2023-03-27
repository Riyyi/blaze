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

	std::string print(ASTNodePtr node, bool print_readably = true);
	std::string printNoErrorCheck(ASTNodePtr node, bool print_readably = true);

private:
	void init();
	void printImpl(ASTNodePtr node, bool print_readably = true);
	void printError();

	bool m_first_node { true };
	bool m_previous_node_is_list { false };
	std::string m_print;
};

} // namespace blaze
