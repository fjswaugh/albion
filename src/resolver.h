#pragma once

#include "ast.h"
#include <unordered_map>

struct ScopeStack;

using Locations = std::unordered_map<const Ast::Variable*, int>;

void resolve(const Ast::Ast&, Locations&);

