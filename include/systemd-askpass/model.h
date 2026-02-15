#ifndef _MODEL_H
#define _MODEL_H

#include <string_view>

#include <sigc++/signal.h>

#include "concepts.h"
#include "exit_codes.h"
#include "systemd-askpass-context.h"

namespace Askpass {
    class Model : public sigc::trackable {
        std::unique_ptr<SystemdAskpassContext> m_context;
        ExitCode m_exit_status {0};

        void on_succeeded(std::string_view input);
        void on_failure();

    public:
        explicit Model(std::unique_ptr<SystemdAskpassContext> context);

        void register_window(WindowInterface auto &window) {
            window.signal_succeeded().connect(sigc::mem_fun(*this, &Model::on_succeeded));
            window.signal_failure().connect(sigc::mem_fun(*this, &Model::on_failure));
        }

        std::string_view get_message() const noexcept { return m_context->message(); }

        constexpr ExitCode exit_status() const noexcept { return m_exit_status; }
    };
} // namespace Askpass

#endif