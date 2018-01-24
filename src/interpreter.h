#pragma once

#include "ast.h"
#include "environment.h"
#include "object.h"
#include "resolver.h"

#include <memory>

struct ReturnValue {
    ObjectReference value;
};

void interpret(const Ast::Ast& ast, const Locations&, std::shared_ptr<Environment> environment,
               std::shared_ptr<Environment> global_environment);

