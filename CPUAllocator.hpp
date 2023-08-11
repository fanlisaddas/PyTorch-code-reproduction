// Copyright fzz 2023
#include "Allocator.hpp"
#include "alloc_cpu.hpp"

namespace c10 {

struct DefaultCPUAllocator final : c10::Allocator {
    DefaultCPUAllocator() = default;
    c10::DataPtr allocate(size_t nbytes) const override {
        void* data = nullptr;
        try {
            data = c10::alloc_cpu(nbytes);
        } catch (std::exception& e) {
            throw e;
        }
        return {data, &Delete};
    }

    static void Delete(void* ptr) {
        if (!ptr) { return; }
        c10::free_cpu(ptr);
    }

    c10::DeleterFnPtr raw_deleter() const override {
        return Delete;
    }
};
}  // namespace c10
