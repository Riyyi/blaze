/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <list>
#include <stack>

#include "env/environment.h"
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
	ValuePtr evalSymbol(ValuePtr ast, EnvironmentPtr env);
	ValuePtr evalVector(ValuePtr ast, EnvironmentPtr env);
	ValuePtr evalHashMap(ValuePtr ast, EnvironmentPtr env);

	ValuePtr evalDef(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalDefMacro(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalFn(const ValueVector& nodes, EnvironmentPtr env);
	ValuePtr evalQuasiQuoteExpand(const ValueVector& nodes);
	ValuePtr evalQuote(const ValueVector& nodes);
	ValuePtr evalTry(const ValueVector& nodes, EnvironmentPtr env);

	void evalDo(const ValueVector& nodes, EnvironmentPtr env);
	void evalIf(const ValueVector& nodes, EnvironmentPtr env);
	void evalLet(const ValueVector& nodes, EnvironmentPtr env);
	void evalMacroExpand1(const ValueVector& nodes, EnvironmentPtr env);
	void evalQuasiQuote(const ValueVector& nodes, EnvironmentPtr env);
	void evalWhile(const ValueVector& nodes, EnvironmentPtr env);

	ValuePtr apply(ValuePtr function, const ValueVector& nodes);

	ValuePtr m_ast;
	EnvironmentPtr m_env;
	EnvironmentPtr m_outer_env;
};

} // namespace blaze
