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

    class Model {
        std::string m_message;

        static void on_succeeded(std::string_view input) {
            std::span<const char> input_span {input.data(), input.size()};
            unbuffered_write_to_stdout(std::as_bytes(input_span));
        }

        static void on_failure() { std::cerr << "Input cancelled by the user\n"; }

    public:
        Model(std::string message) : m_message(std::move(message)) {}

        void register_window(WindowInterface auto &window) {
            window.signal_succeeded().connect(sigc::ptr_fun(on_succeeded));
            window.signal_failure().connect(sigc::ptr_fun(on_failure));
        }

        std::string_view get_message() { return m_message; }
    };

} // namespace Askpass

#endif