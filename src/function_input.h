#pragma once

#include <memory>
#include <cassert>

template <typename T>
struct FunctionInput {
    FunctionInput() = default;
    FunctionInput(T input1)
        : size_{1}
    {
        new (this->get_aligned_buffer(0)) T{std::move(input1)};
    }
    FunctionInput(T input1, T input2)
        : size_{2}
    {
        new (this->get_aligned_buffer(0)) T{std::move(input1)};
        new (this->get_aligned_buffer(1)) T{std::move(input2)};
    }
    FunctionInput(const FunctionInput& fi)
        : size_{fi.size()}
    {
        if (fi.size() > 0) {
            new (this->get_aligned_buffer(0)) T{fi[0]};
        }
        if (fi.size() > 1) {
            new (this->get_aligned_buffer(1)) T{fi[1]};
        }
    }
    FunctionInput& operator=(const FunctionInput&) = delete;
    FunctionInput(FunctionInput&& fi)
        : size_{fi.size()}
    {
        if (fi.size() > 0) {
            new (this->get_aligned_buffer(0)) T{std::move(fi.input1_())};
        }
        if (fi.size() > 1) {
            new (this->get_aligned_buffer(1)) T{std::move(fi.input2_())};
        }
        fi.size_ = 0;
    }
    FunctionInput& operator=(FunctionInput&&) = delete;

    constexpr std::size_t size() const noexcept {
        return size_;
    }

    const T& operator[](std::size_t i) const noexcept {
        assert(this->size() > i);
        return *reinterpret_cast<const T*>(this->get_aligned_buffer(i));
    }
    const T& at(std::size_t i) const noexcept {
        assert(this->size() > i);
        return *reinterpret_cast<const T*>(this->get_aligned_buffer(i));
    }

    ~FunctionInput() noexcept {
        if (this->size() > 0) {
            this->input1_().~T();
        }
        if (this->size() > 1) {
            this->input2_().~T();
        }
    }
private:
    T& input1_() noexcept {
        assert(this->size() > 0);
        return *reinterpret_cast<T*>(this->get_aligned_buffer(0));
    }
    T& input2_() noexcept {
        assert(this->size() > 1);
        return *reinterpret_cast<T*>(this->get_aligned_buffer(1));
    }


    char* get_aligned_buffer(std::size_t i) {
        assert(i == 0 || i == 1);
        return buffer_ + i * width_ + alignof(T) -
               reinterpret_cast<intptr_t>(buffer_) % alignof(T);
    }
    const char* get_aligned_buffer(std::size_t i) const {
        assert(i == 0 || i == 1);
        return buffer_ + i * width_ + alignof(T) -
               reinterpret_cast<intptr_t>(buffer_) % alignof(T);
    }

    static constexpr auto width_ = sizeof(T) + alignof(T);

    char buffer_[width_ * 2] = {};
    std::size_t size_ = 0;
};

