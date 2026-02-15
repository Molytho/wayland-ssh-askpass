#include <fstream>

#include "model.h"
#include "systemd-askpass-context.h"
#include "window.h"

namespace {
    constexpr std::string_view AppId       = "org.molytho.wayland-systemd-askpass";
    constexpr const char AllowedBackends[] = "wayland,x11";
}; // namespace

int main(int argc, char **argv) {
    // Declare the supported options.
    if (argc > 1) {
        Askpass::Model model = [&]() {
            std::ifstream askpass_file(argv[1]);
            auto context = Askpass::SystemdAskpassContext::from_askpass_file(askpass_file);
            return Askpass::Model {std::move(context)};
        }();

        gdk_set_allowed_backends(AllowedBackends);
        auto app = Gtk::Application::create(std::string(AppId));
        if (app->make_window_and_run<Askpass::Window>(0, nullptr, model)) {
            return static_cast<int>(Askpass::ExitCode::Unknown);
        }
        return static_cast<int>(model.exit_status());
    }

    return 0;
}
