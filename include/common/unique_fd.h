#ifndef OWNED_FD_H
#define OWNED_FD_H

#include <utility>

#include <unistd.h>

namespace wrapper {
    class unique_fd {
        int m_fd;

    public:
        constexpr unique_fd() : m_fd(-1) {}
        
        explicit unique_fd(int fd) : m_fd(fd) {}

        ~unique_fd() noexcept { reset(); }

        constexpr unique_fd(unique_fd &&other) noexcept : m_fd(other.m_fd) { other.m_fd = -1; }

        unique_fd &operator=(unique_fd &&other) noexcept {
            swap(*this, other);
            return *this;
        }

        void reset(int fd = -1) noexcept {
            if (m_fd >= 0) {
                close(m_fd);
            }
            m_fd = fd;
        }

        constexpr int get() const noexcept { return m_fd; }

        friend void swap(unique_fd &lhs, unique_fd &rhs) noexcept {
            std::swap(lhs.m_fd, rhs.m_fd);
        }
    };
} // namespace wrapper

#endif