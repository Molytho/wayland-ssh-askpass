#ifndef _EXIT_CODES_H
#define _EXIT_CODES_H

#include <cstdlib>

namespace Askpass {
    enum class ExitCode : int { Success = 0, Cancelled = 1, InvalidPlatform = 254, Unknown = 255 };

    [[noreturn]] inline void exit(ExitCode code) {
        std::exit(static_cast<int>(code));
        std::abort();
    }
} // namespace Askpass

#endif