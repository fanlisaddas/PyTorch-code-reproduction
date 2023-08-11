// Copyright fzz 2023
#ifndef TENSOR_INTRUSIVE_PTR_HPP_
#define TENSOR_INTRUSIVE_PTR_HPP_

#include <atomic>
#include <climits>
#include <memory>
#include <utility>

#include "config.hpp"

namespace c10 {
class intrusive_ptr_target {
 public:
    mutable std::atomic<size_t> refcount_;
    template <typename T, typename NullType>
    friend class intrusive_ptr;

 private:
    // This is called when refcount reaches zero.
    // You can override this to release expensive resources.
    virtual void release_resources() {}

 protected:
    // protected destructor.
    // We never want to destruct intrusive_ptr_target* directly.
    virtual ~intrusive_ptr_target() {}

    constexpr intrusive_ptr_target() noexcept : refcount_(0) {}

    // intrusive_ptr_target supports copy and move:
    // but refcount don't participate
    // (since they are intrinsic properties of the memory location)
    intrusive_ptr_target(const intrusive_ptr_target& other) noexcept
     : intrusive_ptr_target() {}

    intrusive_ptr_target& operator=(
     const intrusive_ptr_target& other) noexcept {
        return *this;
    }

    intrusive_ptr_target(intrusive_ptr_target&& other) noexcept
     : intrusive_ptr_target() {}

    intrusive_ptr_target& operator=(intrusive_ptr_target&& other) noexcept {
        return *this;
    }
};

namespace detail {
template <class TTarget>
struct intrusive_target_default_null_type final {
    static constexpr TTarget* singleton() noexcept {
        return nullptr;
    }
};

template <class TTarget, class ToNullType, class FromNullType>
TTarget* assign_ptr_(TTarget* rhs) {
    if (FromNullType::singleton() == rhs) {
        return ToNullType::singleton();
    } else {
        return rhs;
    }
}

// Increment needs to be acquire-release to make use_count() and
// unique() reliable.
inline size_t atomic_refcount_increment(std::atomic<size_t>& refcount) {
    return refcount.fetch_add(1, std::memory_order_acq_rel) + 1;
}

// Both decrements need to be acquire-release for correctness. See
// e.g. std::shared_ptr implementation.
inline size_t atomic_refcount_decrement(std::atomic<size_t>& refcount) {
    return refcount.fetch_sub(1, std::memory_order_acq_rel) - 1;
}
}  // namespace detail

// partial template
template <
    class TTarget,
    class NullType = detail::intrusive_target_default_null_type<TTarget>>
class intrusive_ptr final {
 private:
    TTarget* target_;
    // check
    static_assert(
        std::is_base_of<
        TTarget,
        typename std::remove_pointer<decltype(NullType::singleton())>::type>::
        value,
        "NullType::singleton() must return a element_type* pointer");

    // increase refcount manually
    void retain_() {
        if (target_ != NullType::singleton()) {
            size_t new_refcount =
             detail::atomic_refcount_increment(target_->refcount_);
            assert(new_refcount != 1,
             "intrusive_ptr: Cannot increase refcount after it reached zero.")
        }
    }

    // delete intrusive_ptr when refcount_ = 0
    void reset_() noexcept {
        if (target_ != NullType::singleton() &&
         detail::atomic_refcount_decrement(target_->refcount_) == 0) {
            delete target_;
        }
    }

    explicit intrusive_ptr(TTarget* target)
     : target_(target) {
        if (target_ != NullType::singleton()) {
            assert(target_->refcount_ == 0,
            "intrusive_ptr: Newly-created target had non-zero refcounts.");
            target_->refcount_.store(1, std::memory_order_relaxed);
        }
    }

 public:
    intrusive_ptr() noexcept : intrusive_ptr(NullType::singleton()) {}

    intrusive_ptr(std::nullptr_t) noexcept
     : intrusive_ptr(NullType::singleton()) {}

    explicit intrusive_ptr(std::unique_ptr<TTarget> rhs) noexcept
     : intrusive_ptr(rhs.release()) {}

    intrusive_ptr(intrusive_ptr&& rhs) noexcept : target_(rhs.target_) {
        rhs.target_ = NullType::singleton();
    }

    template <class From, class FromNullType>
    intrusive_ptr(intrusive_ptr<From, FromNullType>&& rhs) noexcept
     : target_(detail::assign_ptr_<TTarget, NullType, FromNullType>
      (rhs.target_)) {
        static_assert(
            std::is_convertible<From*, TTarget*>::value,
            "Type mismatch. intrusive_ptr move constructor got pointer of wrong type.");
        rhs.target_ = FromNullType::singleton();
    }

    intrusive_ptr(const intrusive_ptr& rhs) : target_(rhs.target_) {
        retain_();
    }

    template <class From, class FromNullType>
    intrusive_ptr(const intrusive_ptr<From, FromNullType>& rhs)
     : target_(detail::assign_ptr_<TTarget, NullType, FromNullType>
      (rhs.target_)) {
        static_assert(
            std::is_convertible<From*, TTarget*>::value,
            "Type mismatch. intrusive_ptr copy constructor got pointer of wrong type.");
        retain_();
    }

    ~intrusive_ptr() noexcept {
         reset_();
    }

    intrusive_ptr& operator=(intrusive_ptr&& rhs) & noexcept {
        return operator=<TTarget, NullType>(std::move(rhs));
    }

    template <class From, class FromNullType>
    intrusive_ptr& operator=(intrusive_ptr<From, FromNullType>&& rhs)
     & noexcept {
        static_assert(
            std::is_convertible<From*, TTarget*>::value,
            "Type mismatch. intrusive_ptr move assignment got pointer of wrong type.");
        intrusive_ptr tmp = std::move(rhs);
        swap(tmp);
        return *this;
    }

    intrusive_ptr& operator=(const intrusive_ptr& rhs) & noexcept {
        return operator=<TTarget, NullType>(rhs);
    }

    template <class From, class FromNullType>
    intrusive_ptr& operator=(const intrusive_ptr<From, NullType>& rhs)& {
        static_assert(
            std::is_convertible<From*, TTarget*>::value,
            "Type mismatch. intrusive_ptr copy assignment got pointer of wrong type.");
        intrusive_ptr tmp = rhs;
        swap(tmp);
        return *this;
    }

    TTarget* get() const noexcept {
        return target_;
    }

    TTarget& operator*() const noexcept {
        return *target_;
    }

    TTarget* operator->() const noexcept {
        return target_;
    }

    operator bool() const noexcept {
        return target_ != NullType::singleton();
    }

    void reset() noexcept {
        reset_();
        target_ = NullType::singleton();
    }

    void swap(intrusive_ptr& rhs) noexcept {
        TTarget* tmp = target_;
        target_ = rhs.target_;
        rhs.target_ = tmp;
    }

    // We do a lot of null-pointer checks in our code,
    // good to have this be cheap.
    bool defined() const noexcept {
        return target_ != NullType::singleton();
    }

    size_t use_count() const noexcept {
        if (target_ == NullType::singleton()) {
            return 0;
        }
        return target_->refcount_.load(std::memory_order_acquire);
    }

    bool unique() const noexcept {
        return use_count() == 1;
    }

    TTarget* release() noexcept {
        TTarget* result = target_;
        target_ = NullType::singleton();
        return result;
    }

	/**
	 * Allocate a heap object with args and wrap it inside a intrusive_ptr and
	 * incref. This is a helper function to let make_intrusive() access private
	 * intrusive_ptr constructors.
	 */
    template <class... Args>
    static intrusive_ptr make(Args&&... args) {
        return intrusive_ptr(new TTarget(std::forward<Args>(args)...));
    }
};

template <
    class TTarget,
    class NullType = detail::intrusive_target_default_null_type<TTarget>,
    class... Args>
inline intrusive_ptr<TTarget, NullType> make_intrusive(Args&&... args) {
    return intrusive_ptr<TTarget, NullType>::make(std::forward<Args>(args)...);
}

template <class TTarget, class NullType>
inline void swap(
    intrusive_ptr<TTarget, NullType>& lhs,
    intrusive_ptr<TTarget, NullType>& rhs) noexcept {
    lhs.swap(rhs);
}

// To allow intrusive_ptr inside std::map or std::set, we need operator<
template <class TTarget1, class NullType1, class TTarget2, class NullType2>
inline bool operator<(
    const intrusive_ptr<TTarget1, NullType1>& lhs,
    const intrusive_ptr<TTarget2, NullType2>& rhs) noexcept {
    return lhs.get() < rhs.get();
}

template <class TTarget1, class NullType1, class TTarget2, class NullType2>
inline bool operator==(
    const intrusive_ptr<TTarget1, NullType1>& lhs,
    const intrusive_ptr<TTarget2, NullType2>& rhs) noexcept {
    return lhs.get() == rhs.get();
}

template <class TTarget1, class NullType1>
inline bool operator==(
    const intrusive_ptr<TTarget1, NullType1>& lhs,
    std::nullptr_t) noexcept {
    return lhs.get() == nullptr;
}

template <class TTarget2, class NullType2>
inline bool operator==(
    std::nullptr_t,
    const intrusive_ptr<TTarget2, NullType2>& rhs) noexcept {
    return nullptr == rhs.get();
}

template <class TTarget1, class NullType1, class TTarget2, class NullType2>
inline bool operator!=(
    const intrusive_ptr<TTarget1, NullType1>& lhs,
    const intrusive_ptr<TTarget2, NullType2>& rhs) noexcept {
    return !operator==(lhs, rhs);
}

template <class TTarget1, class NullType1>
inline bool operator!=(
    const intrusive_ptr<TTarget1, NullType1>& lhs,
    std::nullptr_t) noexcept {
    return !operator==(lhs, nullptr);
}

template <class TTarget2, class NullType2>
inline bool operator!=(
    std::nullptr_t,
    const intrusive_ptr<TTarget2, NullType2>& rhs) noexcept {
    return !operator==(nullptr, rhs);
}
}  // namespace c10

namespace std {
// To allow intrusive_ptr inside std::unordered_map or
// std::unordered_set, we need std::hash
template <class TTarget, class NullType>
struct hash<c10::intrusive_ptr<TTarget, NullType>> {
    size_t operator()(const c10::intrusive_ptr<TTarget, NullType>& x) const {
        return std::hash<TTarget*>()(x.get());
    }
};
}  // namespace std
#endif  // TENSOR_INTRUSIVE_PTR_HPP_
