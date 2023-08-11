// Copyright fzz 2023
#ifndef TENSOR_STORAGEIMPL_HPP_
#define TENSOR_STORAGEIMPL_HPP_

#include <memory>
#include <utility>

#include "Allocator.hpp"
#include "intrusive_ptr.hpp"
#include "config.hpp"

namespace c10 {

struct StorageImpl : public c10::intrusive_ptr_target {
 private:
    DataPtr data_ptr_;
    size_t size_bytes_;
    bool resizable_;
    Allocator* allocator_;

 public:
    StorageImpl(size_t size_bytes, DataPtr data_ptr, c10::Allocator* allocator,
     bool resizable) : data_ptr_(std::move(data_ptr)), size_bytes_(size_bytes),
      resizable_(resizable), allocator_(allocator) {}

    StorageImpl(size_t size_bytes, c10::Allocator* allocator, bool resizable)
     : StorageImpl(size_bytes, allocator->allocate(size_bytes),
      allocator, resizable) {}

    StorageImpl& operator=(StorageImpl&& other) = default;
    StorageImpl& operator=(const StorageImpl&) = delete;
    StorageImpl() = delete;
    StorageImpl(StorageImpl&& other) = default;
    StorageImpl(const StorageImpl&) = delete;
    ~StorageImpl() override = default;

    void reset() {
        data_ptr_ = nullptr;
        size_bytes_ = 0;
    }

    template <typename T>
    inline T* data() const {
        return unsafe_data<T>();
    }

    template <typename T>
    inline T* unsafe_data() const {
        return static_cast<T*>(this->data_ptr_.get());
    }

    // Destructor doesn't call release_resources because it's
    // unnecessary; don't forget to change that if needed!
    void release_resources() override {
        data_ptr_ = nullptr;
    }

    size_t nbytes() const {
        return size_bytes_;
    }

    void set_nbytes(size_t size_bytes) {
        size_bytes_ = size_bytes;
    }

    bool resizable() const {
        return resizable_;
    }

    DataPtr& data_ptr() {
        return data_ptr_;
    }

    const DataPtr& data_ptr() const {
        return data_ptr_;
    }

  // Returns the previous data_ptr
    DataPtr set_data_ptr(DataPtr&& data_ptr) {
        DataPtr old_data_ptr(std::move(data_ptr_));
        data_ptr_ = std::move(data_ptr);
        return old_data_ptr;
    }

    void set_data_ptr_noswap(DataPtr&& data_ptr) {
        data_ptr_ = std::move(data_ptr);
    }

    void* data() {
        return data_ptr_.get();
    }

    void* data() const {
        return data_ptr_.get();
    }

    c10::Allocator* allocator() {
        return allocator_;
    }

    const c10::Allocator* allocator() const {
        return allocator_;
    }

    // You generally shouldn't use this method, but it is occasionally
    // useful if you want to override how a tensor will be reallocated,
    // after it was already allocated (and its initial allocator was
    // set)
    void set_allocator(c10::Allocator* allocator) {
        allocator_ = allocator;
    }

    void set_resizable(bool resizable) {
        if (resizable) {
            // We need an allocator to be resizable
            assert(allocator_,
            "Allocator should not be null when StorageImpl is resizable")
        }
        resizable_ = resizable;
    }
};

}  // namespace c10
#endif  // TENSOR_STORAGEIMPL_HPP_
