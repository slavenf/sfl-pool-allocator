//
// g++ -Wall -Wextra -Wpedantic -std=c++11 -O3 -I ../src ../src/pool_allocator.cpp test_global_container.cpp -o test_global_container
//

#include <vector>

#include "pool_allocator.hpp"

std::vector<int, sfl::pool_allocator<int>> vec;

int main()
{
    vec.push_back(10);
}
