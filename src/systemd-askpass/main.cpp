#include <fstream>

#include "systemd-askpass-context.h"
#include "window-model.h"
#include "window.h"

namespace {
    constexpr std::string_view AppId = "org.molytho.wayland-systemd-askpass";
}; // namespace

int main(int argc, char **argv) {
    if (argc > 1) {
        Askpass::WindowModel model = [&]() {
            std::ifstream askpass_file(argv[1]);
            auto context = Askpass::SystemdAskpassContext::from_askpass_file(askpass_file);
            return Askpass::WindowModel {std::move(context)};
        }();

        Askpass::make_and_run_window(AppId, model);
        return static_cast<int>(model.exit_status());
    }

    return 0;
}
