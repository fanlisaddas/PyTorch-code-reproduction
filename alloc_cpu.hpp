// Copyright fzz 2023
#ifndef TENSOR_ALLOC_CPU_HPP_
#define TENSOR_ALLOC_CPU_HPP_

#include <cstddef>

#include "alloc_cpu.hpp"
#include "config.hpp"

namespace c10 {

constexpr size_t gAlignment = 64;

void* alloc_cpu(size_t nbytes) {
    if (nbytes == 0) {
        return nullptr;
    }
    assert((ptrdiff_t)nbytes >= 0,
     "alloc_cpu() seems to have been called with negative number.");
    void* data;

#ifdef _MSC_VER
    data = _aligned_malloc(nbytes, gAlignment);
    assert(data, "DefaultCPUAllocator: not enough memory.")
#else
    int err = posix_memalign(&data, gAlignment, nbytes);
    assert(err == 0, "DefaultCPUAllocator: not enough memory.")
#endif
    return data;
}

void free_cpu(void* data) {
#ifdef _MSC_VER
    _aligned_free(data);
#else
    free(data);
#endif
}

}  // namespace c10
#endif  // TENSOR_ALLOC_CPU_HPP_
