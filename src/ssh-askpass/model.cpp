#include "model.h"

#include <cerrno>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>

#include <sigc++/signal.h>

#include "macros.h"

namespace {
    void unbuffered_write_to_stdout(std::span<const std::byte> data) {
        while (!data.empty()) {
            size_t byte_to_write = std::min<std::size_t>(data.size(), std::numeric_limits<ssize_t>::max());
            ssize_t result = write(1, data.data(), byte_to_write);
            throw_system_error_if(result < 0 && errno != EAGAIN && errno != EWOULDBLOCK);
            data = data.subspan(result);
        }
    }
} // namespace

namespace Askpass {
    void Model::on_succeeded(std::string_view input) {
        unbuffered_write_to_stdout(std::as_bytes(std::span<const char>(input)));
    }

    void Model::on_failure() {
        std::cerr << "Input cancelled by the user\n";
        m_exit_status = ExitCode::Cancelled;
    }

    Model::Model(std::string message) : m_message(std::move(message)) {}
} // namespace Askpass