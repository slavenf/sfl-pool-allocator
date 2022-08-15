//
// DESCRIPTION:
// Creates std::vector< std::vector<T, sfl::pool_allocator<T>> >.
// Each subvector is the random size.
// Elements in subvectors are random generated.
//
// DEBUG:
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 -I ../src ../src/pool_allocator.cpp test_3b.cpp -o test_3b
//
// RELEASE:
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 -I ../src ../src/pool_allocator.cpp test_3b.cpp -o test_3b -DNDEBUG
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
            // For random subvector element:
            std::uniform_int_distribution<> distrib2(0, 255);

            std::size_t sum = 0;

            benchmark
            (
                "Resizing subvectors and inserting random elements",
                [&]()
                {
                    for (auto& subvec : vec)
                    {
                        subvec.resize(distrib1(gen));

                        for (auto& elem : subvec)
                        {
                            elem = distrib2(gen);
                            sum += elem;
                        }
                    }
                }
            );

            std::size_t control_sum = 0;

            benchmark
            (
                "Accumulating",
                [&]()
                {
                    for (const auto& subvec : vec)
                    {
                        for (const auto& elem : subvec)
                        {
                            control_sum += elem;
                        }
                    }
                }
            );

            if (control_sum != sum)
            {
                std::cout << "ERROR: control_sum != sum" << std::endl;
                std::abort();
            }
        }
    );
}
