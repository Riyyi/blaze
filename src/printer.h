/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "ast.h"

namespace blaze {

// Serializer -> return to string
class Printer {
public:
	Printer(ASTNode* node);
	virtual ~Printer();

	void dump();

private:
	void dumpImpl(ASTNode* node);

	bool m_firstNode { true };
	bool m_previousNodeIsList { false };
	ASTNode* m_node { nullptr };
};

} // namespace blaze
