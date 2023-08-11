// Copyright fzz 2023
#ifndef TENSOR_STORAGE_HPP_
#define TENSOR_STORAGE_HPP_

#include <utility>

#include "StorageImpl.hpp"

namespace c10 {

struct Storage {
 protected:
    c10::intrusive_ptr<StorageImpl> storage_impl_;

 public:
    Storage() = default;
    explicit Storage(c10::intrusive_ptr<StorageImpl> ptr)
     : storage_impl_(std::move(ptr)) {}

    Storage(
     size_t size_bytes, Allocator* allocator = nullptr, bool resizable = false)
     : storage_impl_(c10::make_intrusive<StorageImpl>(
        size_bytes, allocator, resizable)) {}

    Storage(size_t size_bytes, DataPtr data_ptr, Allocator* allocator = nullptr,
     bool resizable = false) : storage_impl_(c10::make_intrusive<StorageImpl>(
      size_bytes, std::move(data_ptr), allocator, resizable)) {}

    template <typename T>
    T* data() const {
        return storage_impl_->data<T>();
    }

    template <typename T>
    T* unsafe_data() const {
        return storage_impl_->unsafe_data<T>();
    }

    void set_nbytes(size_t size_bytes) const {
        storage_impl_.get()->set_nbytes(size_bytes);
    }

    bool resizable() const {
        return storage_impl_->resizable();
    }

    size_t nbytes() const {
        return storage_impl_->nbytes();
    }

    void* data() const {
        return storage_impl_.get()->data();
    }

    c10::DataPtr& data_ptr() {
        return storage_impl_->data_ptr();
    }

    const c10::DataPtr& data_ptr() const {
        return storage_impl_->data_ptr();
    }

    // Returns the previous data_ptr
    c10::DataPtr set_data_ptr(c10::DataPtr&& data_ptr) const {
        return storage_impl_.get()->set_data_ptr(std::move(data_ptr));
    }

    void set_data_ptr_noswap(c10::DataPtr&& data_ptr) const {
        return storage_impl_.get()->set_data_ptr_noswap(std::move(data_ptr));
    }

    c10::Allocator* allocator() const {
        return storage_impl_.get()->allocator();
    }

    StorageImpl* unsafeReleaseStorageImpl() {
        return storage_impl_.release();
    }

    StorageImpl* unsafeGetStorageImpl() const noexcept {
        return storage_impl_.get();
    }

    operator bool() const {
        return storage_impl_;
    }

    size_t use_count() const {
        return storage_impl_.use_count();
    }

    inline bool unique() const {
        return storage_impl_.unique();
    }

    bool is_alias_of(const Storage& other) const {
        return storage_impl_ == other.storage_impl_;
    }
};

}  // namespace c10
#endif  // TENSOR_STORAGE_HPP_
