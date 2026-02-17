#include <iostream>

#include <glib-unix.h>

#include "model.h"
#include "window-model.h"
#include "window.h"

namespace {
    constexpr std::string_view AppId = "org.molytho.wayland-systemd-askpass";
}; // namespace

class UiManager : public sigc::trackable {
    Glib::RefPtr<Gtk::Application> m_application;
    sigc::signal<void(void)> m_window_closed_signal {};
    Gtk::Window *m_open_window {};

    void emit_signal_window_closed() {
        m_open_window = nullptr;
        m_window_closed_signal.emit();
    }

public:
    UiManager() : m_application(Gtk::Application::create(std::string(AppId))) {}

    sigc::signal<void(void)> signal_window_closed() noexcept { return m_window_closed_signal; }

    void spawn_window(Askpass::WindowModel &model) {
        assert(m_open_window == nullptr);
        auto window = Gtk::make_managed<Askpass::Window>(model);
        window->signal_unrealize().connect(sigc::mem_fun(*this, &UiManager::emit_signal_window_closed));
        m_open_window = window;
        m_application->add_window(*window);
        window->present();
    }

    int run(int argc, char *argv[]) {
        m_application->hold();
        return m_application->run(argc, argv);
    }

    void stop() { m_application->release(); }
};

static_assert(Askpass::UiInterface<UiManager>);

int main(int argc, char **argv) {
    UiManager ui_manager {};
    Askpass::Model model {ui_manager};

    auto askpass_dir = Gio::File::create_for_path("/run/user/1000/systemd/ask-password/");
    auto monitor     = askpass_dir->monitor_directory();
    monitor->signal_changed().connect([&]([[maybe_unused]] const Glib::RefPtr<Gio::File> &file1,
                                          [[maybe_unused]] const Glib::RefPtr<Gio::File> &file2,
                                          [[maybe_unused]] Gio::FileMonitor::Event event) {
        std::cout << static_cast<int>(event) << '\n';
        std::cout << file1->get_basename() << '\n';
        if (event == Gio::FileMonitor::Event::CREATED) {
            if (file1->get_basename().starts_with("ask.")) {
                model.on_file_created({file1});
            }
        } else if (event == Gio::FileMonitor::Event::DELETED) {
            if (file1->get_basename().starts_with("ask.")) {
                model.on_file_deleted({file1});
            }
        } else if (event == Gio::FileMonitor::Event::CHANGES_DONE_HINT) {
            model.on_file_events_ended();
        }
        return;
    });

    // Don't need to remove it since ui_manager is alive while the MainLoop runs
    g_unix_signal_add(
        SIGTERM,
        [](gpointer user_data) -> gboolean {
            auto ui_manager = static_cast<UiManager *>(user_data);
            ui_manager->stop();
            return false;
        },
        &ui_manager);

    return ui_manager.run(argc, argv);
}
