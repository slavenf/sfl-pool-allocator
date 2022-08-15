//
// DESCRIPTION:
// Creates std::vector< std::vector<T, std::allocator<T>> >.
// Each subvector is the same size.
// Elements in subvectors are default initialized.
//
// DEBUG:
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 -I ../src ../src/pool_allocator.cpp test_1a.cpp -o test_1a
//
// RELEASE:
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 -I ../src ../src/pool_allocator.cpp test_1a.cpp -o test_1a -DNDEBUG
//

#include <iostream>
#include <vector>

#include "common.hpp"
#include "pool_allocator.hpp"

#define NUM_SUBVECTORS 64*1024*1024
#define SUBVECTOR_SIZE 32

int main()
{
    std::cout << "BEGIN" << std::endl;

    std::vector<std::vector<char>> vec;

    press_any_key_to_continue();

    benchmark
    (
        "Resizing vector",
        [&]()
        {
            vec.resize(NUM_SUBVECTORS);
        }
    );

    press_any_key_to_continue();

    benchmark
    (
        "Resizing subvectors",
        [&]()
        {
            for (auto& subvec : vec)
            {
                subvec.resize(SUBVECTOR_SIZE);
            }
        }
    );

    press_any_key_to_continue();

    if (vec.size() != NUM_SUBVECTORS)
    {
        std::abort();
    }

    std::cout << "THE END" << std::endl;
}
