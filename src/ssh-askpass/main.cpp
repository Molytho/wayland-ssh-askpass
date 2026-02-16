#include <sstream>

#include <gdkmm.h>
#include <gtkmm.h>

#include "model.h"
#include "window.h"

#include <string>

namespace {
    constexpr std::string_view AppId = "org.molytho.wayland-ssh-askpass";
}; // namespace

std::string build_message(int argc, char **argv) {
    std::string str;

    if (argc > 1) {
        std::stringstream message_stream {};
        message_stream << argv[1];
        std::for_each(&argv[2], &argv[argc], [&](const char *str) { message_stream << ' ' << str; });
        str = std::move(message_stream).str();
    }

    if (str.empty()) {
        str = "No message";
    }

    return str;
}

int main(int argc, char **argv) {
    Askpass::Model model = build_message(argc, argv);
    Askpass::make_and_run_window(AppId, model);
    return static_cast<int>(model.exit_status());
}
