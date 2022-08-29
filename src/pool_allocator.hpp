//
// Copyright (c) 2022 Slaven Falandys
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#ifndef SFL_POOL_ALLOCATOR_HPP
#define SFL_POOL_ALLOCATOR_HPP

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <mutex>

#ifndef SFL_POOL_ALLOCATOR_MAX_BLOCK_SIZE
#define SFL_POOL_ALLOCATOR_MAX_BLOCK_SIZE 128
#endif

#if 0
#define SFL_POOL_ALLOCATOR_EXTRA_CHECKS
#endif

#ifdef SFL_POOL_ALLOCATOR_EXTRA_CHECKS
#ifdef NDEBUG
#undef SFL_POOL_ALLOCATOR_EXTRA_CHECKS
#endif
#endif

#define SFL_ASSERT(x) assert(x)

namespace sfl
{

namespace dtl
{

class fixed_size_allocator;

class small_size_allocator
{
private:

    const std::size_t max_block_size_;
    fixed_size_allocator* fixed_size_allocators_;

public:

    small_size_allocator(std::size_t max_block_size);

    ~small_size_allocator() noexcept;

    void* allocate(std::size_t block_size);

    void deallocate(void* p, std::size_t block_size) noexcept;
};

class small_size_allocator_singleton
{
private:

    std::mutex mutex_;
    small_size_allocator alloc_;

private:

    small_size_allocator_singleton()
        : alloc_(SFL_POOL_ALLOCATOR_MAX_BLOCK_SIZE)
    {}

    ~small_size_allocator_singleton() = default;

    small_size_allocator_singleton(const small_size_allocator_singleton&) = delete;
    small_size_allocator_singleton(small_size_allocator_singleton&&) = delete;

    small_size_allocator_singleton& operator=(const small_size_allocator_singleton&) = delete;
    small_size_allocator_singleton& operator=(small_size_allocator_singleton&&) = delete;

public:

    static small_size_allocator_singleton& instance()
    {
        // Meyers singleton. Thread safe in C++11.
        static small_size_allocator_singleton instance;
        return instance;
    }

    void* allocate(std::size_t block_size)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return alloc_.allocate(block_size);
    }

    void deallocate(void* p, std::size_t block_size) noexcept
    {
        std::lock_guard<std::mutex> lock(mutex_);
        alloc_.deallocate(p, block_size);
    }
};

} // namespace dtl

template <typename T>
class pool_allocator
{
public:

    using value_type = T;

    pool_allocator()
    {
        // Call member function instance() to make sure that singleton is
        // created before any container using this allocator is created.
        // This guarantees that singleton's destructor is executed after
        // the last container's destructor is executed, even if some of
        // containers is global or static object.
        ::sfl::dtl::small_size_allocator_singleton::instance(); // Can throw.
    }

    pool_allocator(const pool_allocator&) noexcept
    {}

    template <typename U>
    pool_allocator(const pool_allocator<U>&) noexcept
    {}

    pool_allocator(pool_allocator&&) noexcept
    {}

    template <typename U>
    pool_allocator(pool_allocator<U>&&) noexcept
    {}

    ~pool_allocator() noexcept
    {}

    pool_allocator& operator=(const pool_allocator&) noexcept
    {
        return *this;
    }

    pool_allocator& operator=(pool_allocator&&) noexcept
    {
        return *this;
    }

    T* allocate(std::size_t n, const void* = nullptr)
    {
        void* p = ::sfl::dtl::small_size_allocator_singleton::instance().allocate(
            n * sizeof(T)
        );

        // Alignment check
        SFL_ASSERT(std::size_t(p) % alignof(T) == 0);

        return static_cast<T*>(p);
    }

    void deallocate(T* p, std::size_t n) noexcept
    {
        ::sfl::dtl::small_size_allocator_singleton::instance().deallocate(
            static_cast<void*>(p), n * sizeof(T)
        );
    }
};

template <typename T1, typename T2>
bool operator==(const pool_allocator<T1>&, const pool_allocator<T2>&) noexcept
{
    return true;
}

template <typename T1, typename T2>
bool operator!=(const pool_allocator<T1>&, const pool_allocator<T2>&) noexcept
{
    return false;
}

} // namespace sfl

#ifndef SFL_POOL_ALLOCATOR_DO_NOT_UNDEF_MACROS
#undef SFL_ASSERT
#endif

#endif // SFL_POOL_ALLOCATOR_HPP
