#pragma once

#include "token.h"
#include "error.h"

#include <vector>
#include <functional>

std::vector<Token> scan(const std::string_view source,
                        std::function<void(const ScanError&)> report_error);

