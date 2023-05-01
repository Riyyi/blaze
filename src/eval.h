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

	bool isMacroCall(ValuePtr ast, EnvironmentPtr env);
	ValuePtr macroExpand(ValuePtr ast, EnvironmentPtr env);

	ValuePtr evalDef(const ValueList& nodes, EnvironmentPtr env);
	ValuePtr evalDefMacro(const ValueList& nodes, EnvironmentPtr env);
	ValuePtr evalFn(const ValueList& nodes, EnvironmentPtr env);
	ValuePtr evalMacroExpand(const ValueList& nodes, EnvironmentPtr env);
	ValuePtr evalQuasiQuoteExpand(const ValueList& nodes);
	ValuePtr evalQuote(const ValueList& nodes);
	void evalDo(const ValueList& nodes, EnvironmentPtr env);
	void evalIf(const ValueList& nodes, EnvironmentPtr env);
	void evalLet(const ValueList& nodes, EnvironmentPtr env);
	void evalQuasiQuote(const ValueList& nodes, EnvironmentPtr env);
	void evalTry(const ValueList& nodes, EnvironmentPtr env);

	ValuePtr apply(std::shared_ptr<List> evaluated_list);

	ValuePtr m_ast;
	EnvironmentPtr m_env;

	std::stack<ValuePtr> m_ast_stack;
	std::stack<EnvironmentPtr> m_env_stack;
};

} // namespace blaze
