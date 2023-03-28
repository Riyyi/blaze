/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <list>

#include "ast.h"
#include "environment.h"

namespace blaze {

class Eval {
public:
	Eval(ASTNodePtr ast, EnvironmentPtr env);
	virtual ~Eval() = default;

	void eval();

	ASTNodePtr ast() const { return m_ast; }

private:
	ASTNodePtr evalImpl(ASTNodePtr ast, EnvironmentPtr env);
	ASTNodePtr evalAst(ASTNodePtr ast, EnvironmentPtr env);
	ASTNodePtr evalDef(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env);
	ASTNodePtr evalLet(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env);
	ASTNodePtr evalDo(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env);
	ASTNodePtr evalIf(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env);
	ASTNodePtr apply(std::shared_ptr<List> evaluated_list);

	ASTNodePtr m_ast;
	EnvironmentPtr m_env;
};

} // namespace blaze
