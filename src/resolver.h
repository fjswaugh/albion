#pragma once

#include "ast.h"
#include <unordered_map>

struct ScopeStack;

using Locations = std::unordered_map<const Ast::Variable*, int>;

Locations resolve(const Ast::Ast&);

