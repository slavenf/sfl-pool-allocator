//
// DESCRIPTION:
// Creates std::vector< std::vector<T, std::allocator<T>> >.
// Each subvector is the random size.
// Elements in subvectors are default initialized.
//
// DEBUG:
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 -I ../src ../src/pool_allocator.cpp test_2a.cpp -o test_2a
//
// RELEASE:
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 -I ../src ../src/pool_allocator.cpp test_2a.cpp -o test_2a -DNDEBUG
//

#include <iostream>
#include <random>
#include <vector>

#include "common.hpp"
#include "pool_allocator.hpp"

#define NUM_SUBVECTORS 64*1024*1024
#define SUBVECTOR_SIZE 32

int main()
{
    benchmark
    (
        "Test with std::allocator",
        [&]()
        {
            std::vector<std::vector<char>> vec;

            benchmark
            (
                "Resizing vector",
                [&]()
                {
                    vec.resize(NUM_SUBVECTORS);
                }
            );

            std::random_device rd;
            std::mt19937 gen(rd());
            // For random subvector size:
            std::uniform_int_distribution<> distrib1(1, SUBVECTOR_SIZE);

            benchmark
            (
                "Resizing subvectors",
                [&]()
                {
                    for (auto& subvec : vec)
                    {
                        subvec.resize(distrib1(gen));
                    }
                }
            );

            if (vec.size() != NUM_SUBVECTORS)
            {
                std::abort();
            }
        }
    );
}
