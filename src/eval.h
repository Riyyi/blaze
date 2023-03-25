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
	Eval(ASTNodePtr ast, Environment* env);
	virtual ~Eval() = default;

	void eval();

	ASTNodePtr ast() const { return m_ast; }

private:
	ASTNodePtr evalImpl(ASTNodePtr ast, Environment* env);
	ASTNodePtr evalAst(ASTNodePtr ast, Environment* env);
	ASTNodePtr apply(std::shared_ptr<List> evaluated_list);

	ASTNodePtr m_ast { nullptr };
	Environment* m_env { nullptr };
};

} // namespace blaze
