#pragma once

#include "ast.h"
#include "environment.h"
#include "object.h"

#include <memory>

struct Return_value {
    ObjectReference value;
};

void interpret(const Ast::Ast& ast, std::shared_ptr<Environment> environment);

