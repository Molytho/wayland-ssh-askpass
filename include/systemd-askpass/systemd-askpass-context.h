#ifndef SYSTEMD_ASKPASS_CONTEXT_H
#define SYSTEMD_ASKPASS_CONTEXT_H

#include <ctime>
#include <memory>
#include <string>

#include "unique_fd.h"

namespace Askpass {
    class SystemdAskpassContext {
        std::string m_message;
        int m_pid;
        wrapper::unique_fd m_answer_socket;
        time_t m_timeout;

    public:
        SystemdAskpassContext(std::string m_message, int m_pid, wrapper::unique_fd m_answer_socket, time_t m_timeout);

        constexpr std::string_view message() const noexcept { return m_message; }

        constexpr int pid() const noexcept { return m_pid; }

        constexpr time_t timeout() const noexcept { return m_timeout; }

        constexpr int answer_socket() const noexcept { return m_answer_socket.get(); }

        static std::unique_ptr<SystemdAskpassContext> from_askpass_file(std::basic_istream<char> &askpass_file);
    };
} // namespace Askpass

#endif