#ifndef MACROS_H
#define MACROS_H

#include <cstdlib>
#include <system_error>

#define abort_if(expr) \
    if (expr) {        \
        std::abort();  \
    }

#define throw_system_error_if(expr)                             \
    if (expr) {                                                 \
        throw std::system_error(errno, std::system_category()); \
    }

#endif