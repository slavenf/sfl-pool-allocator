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

#define SFL_POOL_ALLOCATOR_DO_NOT_UNDEF_MACROS
#include "pool_allocator.hpp"

extern "C"
{

#if defined(__linux__) || defined(__unix__)
#include <sys/mman.h>
#elif defined(_WIN32)
#include <memoryapi.h>
#else
#error "Unsupported operating system."
#endif

} // extern "C"

#include <memory>
#include <vector>

namespace sfl
{

namespace dtl
{

#define SFL_BUCKET_SIZE (UINT16_MAX * 2)

class bucket
{
private:

    unsigned char* data_;
    std::uint16_t block_size_;
    std::uint16_t num_blocks_;
    std::uint16_t num_used_blocks_;
    std::uint16_t first_unused_block_;

private:

    /// Access to node in embedded linked list.
    /// Function does not check whether the given block is used or unused.
    ///
    uint16_t& node_in_embedded_list(std::size_t block_idx) const noexcept
    {
        // Pointer to the beginning of the block.
        unsigned char* p = data_ + block_idx * block_size_;

        return *static_cast<std::uint16_t*>
        (
            static_cast<void*>
            (
                // Pointer to node must be aligned to uint16_t.
                std::size_t(p) % 2 == 0 ? p : p + 1
            )
        );
    }

public:

    void init(std::size_t block_size)
    {
        // We are using uint16_t as type for indices in embedded linked list.
        // Because of that, block size cannot be less than 2 bytes.
        block_size_ = block_size < 2 ? 2 : block_size;

        #if defined(__linux__) || defined(__unix__)
        void* p = ::mmap
        (
            nullptr,
            SFL_BUCKET_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0
        );

        if (p == MAP_FAILED)
        {
            throw std::bad_alloc();
        }
        #elif defined(_WIN32)
        void* p = ::VirtualAlloc
        (
            nullptr,
            SFL_BUCKET_SIZE,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE
        );

        if (p == nullptr)
        {
            throw std::bad_alloc();
        }
        #else
        #error "Not implemented."
        #endif

        data_ = static_cast<unsigned char*>(p);

        num_blocks_ = SFL_BUCKET_SIZE / block_size_;

        num_used_blocks_ = 0;

        first_unused_block_ = 0;

        for (std::uint16_t i = 0; i < num_blocks_; ++i)
        {
            node_in_embedded_list(i) = i + 1;
        }
    }

    void release() noexcept
    {
        SFL_ASSERT(data_ != nullptr);
        SFL_ASSERT(num_used_blocks_ == 0);

        #if defined(__linux__) || defined(__unix__)
        ::munmap(static_cast<void*>(data_), SFL_BUCKET_SIZE);
        #elif defined(_WIN32)
        ::VirtualFree(static_cast<void*>(data_), 0, MEM_RELEASE);
        #else
        #error "Not implemented."
        #endif

        data_ = nullptr;
    }

    void* allocate() noexcept
    {
        SFL_ASSERT(data_ != nullptr);
        SFL_ASSERT(num_used_blocks_ < num_blocks_);

        const std::size_t block_idx = first_unused_block_;

        first_unused_block_ = node_in_embedded_list(block_idx);

        ++num_used_blocks_;

        return static_cast<void*>(data_ + block_idx * block_size_);
    }

    void deallocate(void* p) noexcept
    {
        SFL_ASSERT(data_ != nullptr);
        SFL_ASSERT(p >= data_ && p < data_ + SFL_BUCKET_SIZE);

        unsigned char* q = static_cast<unsigned char*>(p);

        // Alignment check.
        SFL_ASSERT((q - data_) % block_size_ == 0);

        const std::size_t block_idx = (q - data_) / block_size_;

        #if 0
        #ifndef NDEBUG
        // Check if block was already deallocated.
        for(auto i = first_unused_block_; i < num_blocks_; i = node_in_embedded_list(i))
        {
            SFL_ASSERT(i != block_idx);
        }
        #endif
        #endif

        node_in_embedded_list(block_idx) = first_unused_block_;

        first_unused_block_ = block_idx;

        --num_used_blocks_;
    }

    bool is_empty() const noexcept
    {
        SFL_ASSERT(data_ != nullptr);
        return num_used_blocks_ == 0;
    }

    bool is_full() const noexcept
    {
        SFL_ASSERT(data_ != nullptr);
        return num_used_blocks_ == num_blocks_;
    }

    bool contains(void* p) const noexcept
    {
        SFL_ASSERT(data_ != nullptr);
        return p >= data_ && p < data_ + SFL_BUCKET_SIZE;
    }
};

class fixed_size_allocator
{
public:

    std::size_t block_size_;
    std::vector<bucket> buckets_;
    bucket* last_alloc_;
    bucket* last_dealloc_;
    bucket* last_empty_;

    void init(std::size_t block_size) noexcept
    {
        block_size_ = block_size;
        last_alloc_ = nullptr;
        last_dealloc_ = nullptr;
        last_empty_ = nullptr;
    }

    void release() noexcept
    {
        for (auto& b : buckets_)
        {
            b.release();
        }
        buckets_.clear();
        last_alloc_ = nullptr;
        last_dealloc_ = nullptr;
        last_empty_ = nullptr;
    }

    void* allocate()
    {
        if (last_alloc_ == nullptr || last_alloc_->is_full())
        {
            auto it = buckets_.begin();
            while (it != buckets_.end())
            {
                if (!it->is_full())
                {
                    break;
                }
                ++it;
            }

            if (it != buckets_.end())
            {
                last_alloc_ = std::addressof(*it);
            }
            else
            {
                bucket b;
                b.init(block_size_); // Can throw. No effects if throws.

                try
                {
                    buckets_.emplace_back(b); // Can throw. No effects if throws.
                }
                catch (...)
                {
                    b.release();
                    throw;
                }

                last_alloc_ = std::addressof(buckets_.back());
                last_dealloc_ = nullptr;
                last_empty_ = nullptr;
            }
        }

        if (last_alloc_ == last_empty_)
        {
            last_empty_ = nullptr;
        }

        return last_alloc_->allocate();
    }

    void deallocate(void* p) noexcept
    {
        if (last_dealloc_ == nullptr || !last_dealloc_->contains(p))
        {
            auto it = buckets_.begin();
            while (it != buckets_.end())
            {
                if (it->contains(p))
                {
                    break;
                }
                ++it;
            }

            SFL_ASSERT(it != buckets_.end());
            last_dealloc_ = std::addressof(*it);
        }

        last_dealloc_->deallocate(p);

        if (last_dealloc_->is_empty())
        {
            if (last_empty_ != nullptr)
            {
                SFL_ASSERT(last_empty_ == std::addressof(buckets_.back()));
                buckets_.pop_back();
            }

            using std::swap;
            swap(*last_dealloc_, buckets_.back());
            last_alloc_ = nullptr; // TODO: Improve.
            last_dealloc_ = std::addressof(buckets_.back());
            last_empty_ = last_dealloc_;
        }
    }
};

small_size_allocator::small_size_allocator(std::size_t max_block_size)
    : max_block_size_(max_block_size)
    , fixed_size_allocators_(new fixed_size_allocator[max_block_size]) // Can throw.
{
    for (std::size_t i = 0; i < max_block_size_; ++i)
    {
        fixed_size_allocators_[i].init(i + 1);
    }
}

small_size_allocator::~small_size_allocator() noexcept
{
    for (std::size_t i = 0; i < max_block_size_; ++i)
    {
        fixed_size_allocators_[i].release();
    }

    delete[] fixed_size_allocators_;
}

void* small_size_allocator::allocate(std::size_t block_size)
{
    if (block_size > max_block_size_)
    {
        return ::operator new(block_size); // Can throw.
    }
    else
    {
        const std::size_t index = block_size - 1;
        return fixed_size_allocators_[index].allocate(); // Can throw.
    }
}

void small_size_allocator::deallocate(void* p, std::size_t block_size) noexcept
{
    if (block_size > max_block_size_)
    {
        ::operator delete(p);
    }
    else
    {
        const std::size_t index = block_size - 1;
        fixed_size_allocators_[index].deallocate(p);
    }
}

} // namespace dtl

} // namespace sfl
