#ifndef _MODEL_H
#define _MODEL_H

#include <string>
#include <string_view>

#include <sigc++/signal.h>

#include "exit_codes.h"
#include "window.h"

namespace Askpass {
    template<class T>
    concept WindowInterface = requires(T &obj) {
        { obj.signal_succeeded() } -> std::same_as<sigc::signal<on_succeeded_func_t>>;
        { obj.signal_failure() } -> std::same_as<sigc::signal<on_failure_func_t>>;
    };

    class Model : public sigc::trackable {
        std::string m_message;
        ExitCode m_exit_status {0};

        static void on_succeeded(std::string_view input);
        void on_failure();

    public:
        Model(std::string message);

        void register_window(WindowInterface auto &window) {
            window.signal_succeeded().connect(sigc::ptr_fun(on_succeeded));
            window.signal_failure().connect(sigc::mem_fun(*this, &Model::on_failure));
        }

        constexpr std::string_view get_message() const noexcept { return m_message; }

        constexpr ExitCode exit_status() const noexcept { return m_exit_status; }
    };
} // namespace Askpass

#endif