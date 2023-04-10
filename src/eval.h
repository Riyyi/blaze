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
	Eval(ValuePtr ast, EnvironmentPtr env);
	virtual ~Eval() = default;

	void eval();

	ValuePtr ast() const { return m_ast; }

private:
	ValuePtr evalImpl();
	ValuePtr evalAst(ValuePtr ast, EnvironmentPtr env);

	ValuePtr evalDef(const std::list<ValuePtr>& nodes, EnvironmentPtr env);
	void evalLet(const std::list<ValuePtr>& nodes, EnvironmentPtr env);
	ValuePtr evalQuote(const std::list<ValuePtr>& nodes);
	void evalQuasiQuote(const std::list<ValuePtr>& nodes, EnvironmentPtr env);
	ValuePtr evalQuasiQuoteExpand(const std::list<ValuePtr>& nodes, EnvironmentPtr env);
	void evalDo(const std::list<ValuePtr>& nodes, EnvironmentPtr env);
	void evalIf(const std::list<ValuePtr>& nodes, EnvironmentPtr env);
	ValuePtr evalFn(const std::list<ValuePtr>& nodes, EnvironmentPtr env);

	ValuePtr apply(std::shared_ptr<List> evaluated_list);

	ValuePtr m_ast;
	EnvironmentPtr m_env;

	std::stack<ValuePtr> m_ast_stack;
	std::stack<EnvironmentPtr> m_env_stack;
};

} // namespace blaze
