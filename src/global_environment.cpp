#include "environment.h"

#include <chrono>
#include <iostream>
#include <iterator>
#include <fstream>

std::shared_ptr<Environment> global_environment = [] {
    auto ge = std::make_shared<Environment>(nullptr);
    const auto begin = std::chrono::high_resolution_clock::now();

    auto clock = BuiltInFunction{
        "clock",
        [&begin](const FunctionInput<ObjectReference>&, const Token&) -> ObjectReference {
            auto now = std::chrono::high_resolution_clock::now();
            return static_cast<double>(
                std::chrono::duration_cast<std::chrono::milliseconds>(now - begin).count());
        }
    };
    auto read = BuiltInFunction{
        "read",
        [](const FunctionInput<ObjectReference>& input, const Token&) -> ObjectReference {
            if (input.size() == 0) {
                std::string s;
                std::getline(std::cin, s);
                return s;
            } else {
                if (!input[0].holds<std::string>()) return nullptr;

                const auto& s = input[0].get<std::string>();
                std::ifstream ifs{s};
                std::string text(std::istreambuf_iterator<char>{ifs}, {});
                return text;
            }
        }
    };
    auto print = BuiltInFunction{
        "print",
        [](const FunctionInput<ObjectReference>& input, const Token&) -> ObjectReference {
            if (input.size() == 0) std::cout << '\n';
            else std::cout << to_string(input[0]) << '\n';
            return nullptr;
        }
    };

    ge->define("clock", std::move(clock));
    ge->define("read", std::move(read));
    ge->define("print", std::move(print));
    return ge;
}();

