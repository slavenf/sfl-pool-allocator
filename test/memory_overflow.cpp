//
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 memory_overflow.cpp -o memory_overflow
//

extern "C"
{
#if defined(__linux__) || defined(__unix__)
#include <sys/mman.h>
#elif defined(_WIN32)
#include <memoryapi.h>
#else
#error "Unsupported operating system."
#endif
}

#include <cerrno>
#include <cstring>
#include <iostream>
#include <vector>

using std::cout;
using std::endl;

std::string readable_size(double size)
{
    int i = 0;
    const char* units[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    while (size > 1024)
    {
        size /= 1024;
        i++;
    }
    return std::to_string(size) + units[i];
}

int main()
{
    const std::size_t BLOCK_SIZE = 64*1024;
    const std::size_t NUM_BLOCKS = 1000000;

    std::vector<void*> vec;
    vec.reserve(NUM_BLOCKS);

    for (std::size_t i = 0; i < NUM_BLOCKS; ++i)
    {
        cout << "i: " << i << ", size: "
             << readable_size((i + 1) * BLOCK_SIZE) << endl;

        #if defined(__linux__) || defined(__unix__)
        void* p = ::mmap
        (
            nullptr,
            BLOCK_SIZE,
            PROT_READ | PROT_WRITE,
            MAP_PRIVATE | MAP_ANONYMOUS,
            -1,
            0
        );

        if (p == MAP_FAILED)
        {
            cout << "mmap failed; errno: " << errno
                 << "; description: " << std::strerror(errno) << endl;
            break;
        }
        #elif defined(_WIN32)
        void* p = ::VirtualAlloc
        (
            nullptr,
            BLOCK_SIZE,
            MEM_RESERVE | MEM_COMMIT,
            PAGE_READWRITE
        );

        if (p == nullptr)
        {
            cout << "VirtualAlloc failed" << endl;
            break;
        }
        #else
        #error "Unsupported operating system."
        #endif

        vec.push_back(p);

        for (std::size_t i = 0; i < BLOCK_SIZE; ++i)
        {
            *(static_cast<unsigned char*>(p) + i) = i;
        }
    }

    std::size_t sum = 0;

    for (auto& p : vec)
    {
        for (std::size_t i = 0; i < BLOCK_SIZE; ++i)
        {
            sum += *(static_cast<unsigned char*>(p) + i);
        }
    }

    cout << "sum: " << sum << endl;

    for (auto& p : vec)
    {
        #if defined(__linux__) || defined(__unix__)
        ::munmap(p, BLOCK_SIZE);
        #elif defined(_WIN32)
        ::VirtualFree(p, 0, MEM_RELEASE);
        #else
        #error "Unsupported operating system."
        #endif
    }

    cout << "THE END" << endl;
}
