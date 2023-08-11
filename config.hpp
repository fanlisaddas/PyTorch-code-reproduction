// Copyright fzz 2023
#ifndef TENSOR_CONFIG_HPP_
#define TENSOR_CONFIG_HPP_

#include <iostream>

#define CHECK_NOT_NULL(ptr) \
    do { \
        if ((ptr) == nullptr) { \
            std::cout << "failed to allocate memory." << size << std::endl; \
            exit(EXIT_FAILURE); \
        } else { \
            std::cout << "Memory allocation successful." << std::endl; \
        } \
    } while (false)

#define assert(expression, message) \
    if (!(expression)) { \
        std::cerr << "Assertion failed: " << (message) \
                  << ", file " << __FILE__ << ", line "  \
                  << __LINE__ << std::endl; \
        std::terminate(); \
    }
#endif  // TENSOR_CONFIG_HPP_
