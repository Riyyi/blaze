/*
 * Copyright (C) 2023 Riyyi
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory> // std::shared_ptr

namespace blaze {

class ASTNode;
typedef std::shared_ptr<ASTNode> ASTNodePtr;

class Environment;
typedef std::shared_ptr<Environment> EnvironmentPtr;

} // namespace blaze
