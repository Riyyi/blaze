/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include "ast.h"
#include "environment.h"

namespace blaze {

class Eval {
public:
	Eval(ASTNode* ast, Environment* env);
	virtual ~Eval() = default;

	void eval();

	ASTNode* ast() const { return m_ast; }

private:
	ASTNode* evalImpl(ASTNode* ast, Environment* env);
	ASTNode* evalAst(ASTNode* ast, Environment* env);
	ASTNode* apply(List* evaluated_list);

	ASTNode* m_ast { nullptr };
	Environment* m_env { nullptr };
};

} // namespace blaze
