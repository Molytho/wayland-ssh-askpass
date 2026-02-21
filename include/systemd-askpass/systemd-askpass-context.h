#ifndef SYSTEMD_ASKPASS_CONTEXT_H
#define SYSTEMD_ASKPASS_CONTEXT_H

#include <ctime>
#include <memory>
#include <string>

#include "unique_fd.h"

namespace Askpass {
    class SystemdAskpassContext {
        std::string m_message;
        int m_pid;     // TODO
        wrapper::unique_fd m_answer_socket;
        time_t m_timeout; // TODO

    public:
        SystemdAskpassContext(std::string m_message, int m_pid, wrapper::unique_fd m_answer_socket, time_t m_timeout);

        constexpr std::string_view message() const noexcept { return m_message; }

        constexpr int answer_socket() const noexcept { return m_answer_socket.get(); }

        static std::unique_ptr<SystemdAskpassContext> from_askpass_file(std::basic_istream<char> &askpass_file);
    };
} // namespace Askpass

#endif