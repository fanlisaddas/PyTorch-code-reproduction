// Copyright fzz 2023
#ifndef TENSOR_ALLOCATOR_HPP_
#define TENSOR_ALLOCATOR_HPP_

#include <memory>
#include <utility>

#include "config.hpp"

namespace c10 {

using DeleterFnPtr = void (*)(void*);
using DataPtr = std::unique_ptr<void, DeleterFnPtr>;

struct Allocator {
    virtual ~Allocator() = default;

    virtual DataPtr allocate(size_t n) const = 0;

    virtual DeleterFnPtr raw_deleter() const {
        return nullptr;
    }

    void* raw_allocate(size_t n) {
        auto dptr = allocate(n);
        return dptr.release();
    }

    void raw_deallocate(void* ptr) {
        auto d = raw_deleter();
        assert(d != nullptr, "Deleter shoule not be nullptr.");
        d(ptr);
    }
};

}  // namespace c10
#endif  // TENSOR_ALLOCATOR_HPP_
