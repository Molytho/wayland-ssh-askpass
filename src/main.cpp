#include <sstream>

#include <gdkmm.h>
#include <gtkmm.h>

#include "model.h"
#include "window.h"

#include <string>

namespace {
    constexpr std::string_view AppId       = "org.molytho.wayland-ssh-askpass";
    constexpr const char AllowedBackends[] = "wayland,x11";
}; // namespace

std::string build_message(int argc, char **argv) {
    if (argc == 1) {
        return "Caller did not provide message";
    } else {
        std::stringstream message_stream {};
        message_stream << argv[1];
        std::for_each(&argv[2], &argv[argc], [&](const char *str) { message_stream << ' ' << str; });
        return std::move(message_stream).str();
    }
}

int main(int argc, char **argv) {
    gdk_set_allowed_backends(AllowedBackends);
    auto app = Gtk::Application::create(std::string(AppId));
    Askpass::Model model = build_message(argc, argv);
    if (app->make_window_and_run<Askpass::Window>(0, nullptr, model)) {
        return static_cast<int>(Askpass::ExitCode::Unknown);
    }
    return static_cast<int>(model.exit_status());
}
