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

	ValuePtr evalDef(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalDefMacro(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalFn(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalMacroExpand(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalQuasiQuoteExpand(const ValueVector& nodes);
	ValuePtr evalQuote(const ValueVector& nodes);
	void evalDo(const ValueVector& nodes, EnvironmentPtr env);
	void evalIf(const ValueVector& nodes, EnvironmentPtr env);
	void evalLet(const ValueVector& nodes, EnvironmentPtr env);
	void evalQuasiQuote(const ValueVector& nodes, EnvironmentPtr env);
	void evalTry(const ValueVector& nodes, EnvironmentPtr env);

	ValuePtr apply(std::shared_ptr<List> evaluated_list);

	ValuePtr m_ast;
	EnvironmentPtr m_env;
	EnvironmentPtr m_outer_env;
};

} // namespace blaze
