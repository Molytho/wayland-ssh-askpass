#include "model.h"

#include <span>
#include <string_view>
#include <sys/uio.h>
#include <system_error>

#include <sigc++/signal.h>

#include <sys/socket.h>

namespace {
    void write_answer(int socket_fd, bool success, std::span<const std::byte> data) {
        char is_successful_character = success ? '+' : '-';
        std::array vecs              = {
            iovec {&is_successful_character,             sizeof(is_successful_character)},
            iovec {const_cast<std::byte *>(data.data()), data.size()                    }
        };

        ssize_t res = writev(socket_fd, vecs.data(), vecs.size());
        if (res < 0) {
            throw std::system_error(errno, std::system_category());
        }
    }

    void write_answer(int socket_fd, bool success, std::string_view password) {
        write_answer(socket_fd, success, std::as_bytes(std::span(password)));
    }

} // namespace

namespace Askpass {
    void Model::on_succeeded(std::string_view input) {
        write_answer(m_context->answer_socket(), true, input);
        m_exit_status = ExitCode::Success;
    }

    void Model::on_failure() {
        write_answer(m_context->answer_socket(), false, std::string_view {});
        m_exit_status = ExitCode::Cancelled;
    }

    Model::Model(std::unique_ptr<SystemdAskpassContext> context) : m_context(std::move(context)) {}
} // namespace Askpass