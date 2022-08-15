//
// DESCRIPTION:
// Creates std::vector< std::vector<T, sfl::pool_allocator<T>> >.
// Each subvector is the random size.
// Elements in subvectors are default initialized.
//
// DEBUG:
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 -I ../src ../src/pool_allocator.cpp test_2b.cpp -o test_2b
//
// RELEASE:
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 -I ../src ../src/pool_allocator.cpp test_2b.cpp -o test_2b -DNDEBUG
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
        "Test with sfl::pool_allocator",
        [&]()
        {
            std::vector<std::vector<char, sfl::pool_allocator<char>>> vec;

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
