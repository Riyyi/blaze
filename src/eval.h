/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <list>
#include <stack>

#include "environment.h"
#include "forward.h"

namespace blaze {

class List;

class Eval {
public:
	Eval(ASTNodePtr ast, EnvironmentPtr env);
	virtual ~Eval() = default;

	void eval();

	ASTNodePtr ast() const { return m_ast; }

private:
	ASTNodePtr evalImpl();
	ASTNodePtr evalAst(ASTNodePtr ast, EnvironmentPtr env);
	ASTNodePtr evalDef(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env);
	void evalLet(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env);
	void evalDo(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env);
	void evalIf(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env);
	ASTNodePtr evalFn(const std::list<ASTNodePtr>& nodes, EnvironmentPtr env);
	ASTNodePtr apply(std::shared_ptr<List> evaluated_list);

	ASTNodePtr m_ast;
	EnvironmentPtr m_env;

	std::stack<ASTNodePtr> m_ast_stack;
	std::stack<EnvironmentPtr> m_env_stack;
};

} // namespace blaze
