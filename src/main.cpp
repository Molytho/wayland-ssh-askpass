#include "model.hpp"
#include "window.hpp"

#include <sstream>

#include <gdkmm.h>
#include <gtkmm.h>

namespace {
    constexpr std::string_view APP_ID       = "org.molytho.wayland-ssh-askpass";
    constexpr const char ALLOWED_BACKENDS[] = "wayland,x11";
}; // namespace

int main(int argc, char **argv) {
    gdk_set_allowed_backends(ALLOWED_BACKENDS);
    auto app   = Gtk::Application::create(std::string(APP_ID));
    auto model = [&]() -> Askpass::Model {
        if (argc == 1) {
            return {"Caller did not provide message"};
        } else {
            std::stringstream message_stream;
            message_stream << argv[1];
            std::for_each(&argv[2], &argv[argc], [&](const char *str) {
                message_stream << ' ' << str;
            });
            return {message_stream.str()};
        }
    }();
    return app->make_window_and_run<Askpass::Window>(0, nullptr, model);
}
