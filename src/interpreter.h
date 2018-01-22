#pragma once

#include "ast.h"
#include "environment.h"
#include "object.h"
#include "resolver.h"

#include <memory>

struct ReturnValue {
    ObjectReference value;
};

void interpret(const Ast::Ast& ast, std::shared_ptr<Environment> environment,
               const Locations& locations);

