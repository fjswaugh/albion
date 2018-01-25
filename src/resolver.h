#pragma once

#include "ast.h"
#include <unordered_map>
#include <string>
#include <unordered_set>

struct ScopeStack;

using Locations = std::unordered_map<std::uint64_t, int>;

struct Scope {
    Scope() = default;

    void define(const Ast::VariableTuple&);
    bool has_defined(const Ast::Variable&) const;

    void combine_with(const Scope&);
private:
    std::unordered_set<std::string> data_;
};

struct ScopeStack {
    ScopeStack() { this->push(); }

    auto size() const { return data_.size(); }

    void push() { data_.emplace_back(); }
    void pop() { data_.pop_back(); }

    Scope& top() { assert((!data_.empty())); return data_.back(); }
    const Scope& top() const { assert((!data_.empty())); return data_.back(); }

    Scope& bottom() { assert((!data_.empty())); return data_.front(); }
    const Scope& bottom() const { assert((!data_.empty())); return data_.front(); }

    std::optional<int> resolve(const Ast::Variable&) const;
private:
    Scope& at(std::size_t i) { return data_[data_.size() - i - 1]; }
    const Scope& at(std::size_t i) const { return data_[data_.size() - i - 1]; }

    std::vector<Scope> data_;
};

void resolve(const Ast::Ast&, ScopeStack&, Locations&);

