#ifndef _MODEL_HPP
#define _MODEL_HPP

#include <cassert>
#include <cerrno>
#include <iostream>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <system_error>

#include <sigc++/signal.h>

namespace Askpass {

    using on_succeeded_func_t = void(const std::string_view &input);
    using on_failure_func_t   = void();

    template<class T>
    concept WindowInterface = requires(T &obj) {
        { obj.signal_succeeded() } -> std::same_as<sigc::signal<on_succeeded_func_t>>;
        { obj.signal_failure() } -> std::same_as<sigc::signal<on_failure_func_t>>;
    };

    inline void unbuffered_write_to_stdout(std::span<const std::byte> data) {
        assert(data.size() <= std::numeric_limits<ssize_t>::max());
        size_t written = 0;
        while (written < data.size()) {
            ssize_t result = write(1, data.data() + written, data.size() - written);
            if (result < 0) {
                throw std::system_error(errno, std::system_category());
            }
            written += static_cast<size_t>(result);
        }
    }

    class Model : public sigc::trackable {
        std::string m_message;
        int m_exit_status {0};

        static void on_succeeded(std::string_view input) {
            std::span<const char> input_span {input.data(), input.size()};
            unbuffered_write_to_stdout(std::as_bytes(input_span));
        }

        void on_failure() {
            std::cerr << "Input cancelled by the user\n";
            m_exit_status = 1;
        }

    public:
        Model(std::string message) : m_message(std::move(message)) {}

        void register_window(WindowInterface auto &window) {
            window.signal_succeeded().connect(sigc::ptr_fun(on_succeeded));
            window.signal_failure().connect(sigc::mem_fun(*this, &Model::on_failure));
        }

        std::string_view get_message() const noexcept { return m_message; }

        int exit_status() const noexcept { return m_exit_status; }
    };

} // namespace Askpass

#endif