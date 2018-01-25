#pragma once

#include "token.h"
#include "ast.h"
#include "error.h"

#include <functional>
#include <vector>

Ast::Ast parse(const std::vector<Token>& tokens, std::function<void(const Error&)> report_error);
