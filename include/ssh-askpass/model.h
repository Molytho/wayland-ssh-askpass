#ifndef _MODEL_H
#define _MODEL_H

#include <string>
#include <string_view>

#include <sigc++/signal.h>

#include "concepts.h"
#include "exit_codes.h"

namespace Askpass {
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

        constexpr std::string_view message() const noexcept { return m_message; }

        constexpr ExitCode exit_status() const noexcept { return m_exit_status; }
    };
} // namespace Askpass

#endif