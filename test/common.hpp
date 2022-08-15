#ifndef SFL_TEST_COMMON_HPP
#define SFL_TEST_COMMON_HPP

#include <chrono>
#include <cstdlib>
#include <iostream>

inline void press_any_key_to_continue()
{
    #if defined(__linux__) || defined(__unix__)
    std::cout << "Press enter to continue." << std::endl;
    std::system("read");
    #elif defined(_WIN32)
    std::system("pause");
    #else
    #error "Not implemented."
    #endif
}

template <typename Message, typename Callable>
void benchmark(const Message& message, Callable callable)
{
    std::cout << "Start: " << message << std::endl;
    const auto t1 = std::chrono::steady_clock::now();
    callable();
    const auto t2 = std::chrono::steady_clock::now();
    const std::chrono::duration<double> diff = t2 - t1;
    std::cout << "End:   " << message
              << " (duration: " << diff.count() << " sec)" << std::endl;
}

#endif // SFL_TEST_COMMON_HPP
