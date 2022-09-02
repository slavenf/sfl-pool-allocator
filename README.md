# sfl::pool_allocator

The `sfl::pool_allocator` is a C++11 memory allocator based on memory pools.
It offers fast and efficient allocation of a large number of small-size objects.

## Memory pool organization

This memory allocator uses memory *pool* which is organized in the form of *buckets*.
Bucket is larger fixed-size chunk of memory.
Each bucket consists of integral number of fixed-size memory *blocks*.
Buckets keep tracks of used (i.e. allocated) and unused (i.e. free) blocks
without any memory overhead by embedding linked list inside unused blocks.
Block is allocated by removing it from linked list and deallocated by adding
it back into the linked list which avoids using `malloc`/`free` or `::operator new`/`delete`.

All buckets are the same size (128 KiB each).
Buckets are specialized for memory blocks of one size.
The number of blocks in the bucket depends on the block size.
The larger the block size, the smaller the number of blocks in a bucket.

All buckets are perfectly aligned to memory pages.
On Linux and Unix buckets are allocated by function `mmap` from header `<sys/mman.h>`.
On Windows buckets are allocated by function `VirtualAlloc` from header `<memoryapi.h>`.
Both `mmap` and `VirtualAlloc` allocate memory starting at the beginning of the
memory page and both functions allocate integral number of memory pages.

Having equal-in-size and page-aligned buckets, destruction of one bucket creates a
place suitable for construction of another bucket which can be specialized for
different block size.

When a memory allocation request comes, the allocator searches for available
bucket matching requested block size and allocates block from that bucket.
If all buckets matching requested block size are full, the allocator creates
the new bucket, initializes it for requested block size and allocates a block
from it.
If requested block size is too large, the allocator dispatches memory
allocation request to the default `::operator new`.

When a memory deallocation request comes, the allocator finds corresponding
bucket and marks block in that bucket as available for future allocations.
The allocator keep tracks of empty buckets and destroys them.
Destroying empty buckets the allocator reduces memory consumption and makes
place suitable for creation of new buckets.

## Class template sfl::pool_allocator

Defined in header `pool_allocator.hpp`:

```txt
namespace sfl {

template <typename T>
class pool_allocator;

}
```

`sfl::pool_allocator` is class template that meets requirements of
[`Allocator`](https://en.cppreference.com/w/cpp/named_req/Allocator).
Optional requirements are not implemented because all allocator-aware classes,
including standard library containers, access allocators indirectly through
`std::allocator_traits`, and `std::allocator_traits` supplies the default
implementation of those requirements.

All instances of `sfl::pool_allocator` use the same memory pool.

All instances of `sfl::pool_allocator` are thread safe.

# Requirements

1. Linux, Unix or Windows operating system.
2. C++11 compiler or newer.

# Installation

Copy files `pool_allocator.hpp` and `pool_allocator.cpp` from directory `src`
into your project directory and compile together with your project.

# Usage

Use `sfl::pool_allocator` as a drop-in replacement for `std::allocator`.

Consider using `std::vector<T, sfl::pool_allocator<T>>` instead of `std::vector<T>`
when the capacity of vector is small and the size of `T` is small.
Memory for vector will be allocated from the pool if the capacity of vector
multiplied by size of `T` is less than the maximal block size that
memory pool is configured for.

Also consider using `sfl::pool_allocator` when using `std::deque`, `std::list`,
`std::map`, `std::set` and other associative containers.

# Configuration

All instances of `sfl::pool_allocator` are using the same memory pool
which is by default configured for maximal block size of 128 bytes.
This value is controlled by macro `SFL_POOL_ALLOCATOR_MAX_BLOCK_SIZE`.
The value of this macro can be changed in three different ways:

1.  Invoke compiler with appropriate flag.
    For example, to set maximal block size to 256 bytes:

    ```txt
    $ g++ -D SFL_POOL_ALLOCATOR_MAX_BLOCK_SIZE=256 ......
    ```

2.  Define `SFL_POOL_ALLOCATOR_MAX_BLOCK_SIZE` before including `pool_allocator.hpp`.

    This must be done at all places where `pool_allocator.hpp` is included.

    Expect undefined behavior if you forget to define that macro at all places.

    For example, to set maximal block size to 256 bytes:

    ```txt
    #define SFL_POOL_ALLOCATOR_MAX_BLOCK_SIZE 256
    #include "pool_allocator.hpp"
    ```

3.  Modify value of `SFL_POOL_ALLOCATOR_MAX_BLOCK_SIZE` in file `pool_allocator.hpp`.

    I do not recommend this because you have to modify this value every time
    you update this library.

# Exceptions

This library throws exceptions in case of errors.
The most common exceptions are of type `std::bad_alloc` but some other
exceptions of type derived from `std::exception` could also be throw.

# Debugging

This library extensively uses macro `assert` from header `<cassert>`.

The definition of the macro `assert` depends on another macro, `NDEBUG`,
which is not defined by the standard library.

If `NDEBUG` is defined then `assert` does nothing.

If `NDEBUG` is not defined then `assert` performs check.
If check fails, `assert` outputs implementation-specific diagnostic
information on the standard error output and calls `std::abort`.

Extra checks can be enabled by defining macro `SFL_POOL_ALLOCATOR_EXTRA_CHECKS`.
Enable extra checks if you suspect on *double free* error.
Keep in mind that extra checks are extra expensive (in time).

If `NDEBUG` is defined then extra checks are not enabled.

# Tests

Directory `test` contains test programs.
If you like to compile those tests, you have to do it manually.
There are no makefiles but I am going to create makefiles in the future.
At the top of each file is build command for GCC.

# License

Licensed under zlib license. The license text is in [`LICENSE.txt`](LICENSE.txt) file.
