#include <filesystem>
#include <string_view>

#include <glib-unix.h>

#include "model.h"
#include "window-model.h"
#include "window.h"

namespace {
    constexpr std::string_view AppId       = "org.molytho.wayland-systemd-askpass";
    constexpr char XdgRuntimeDirVariable[] = "XDG_RUNTIME_DIR";

    std::string_view get_xdg_runtime_dir() {
        return getenv(XdgRuntimeDirVariable);
    }
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

    void close_window() {
        m_open_window->close();
    }

    sigc::connection set_timeout(unsigned int milliseconds, const sigc::slot<bool()> &func) {
        return Glib::signal_timeout().connect(func, milliseconds);
    }

    int run(int argc, char *argv[]) {
        m_application->hold();
        return m_application->run(argc, argv);
    }

    void stop() { m_application->release(); }
};

static_assert(Askpass::UiInterface<UiManager>);

auto get_askpass_directory_gio_file() {
    std::filesystem::path runtime_dir = get_xdg_runtime_dir();
    if (runtime_dir.empty()) {
        exit(Askpass::ExitCode::RuntimeDirectoryUnset);
    }
    runtime_dir /= "systemd/ask-password";

    return Gio::File::create_for_path(runtime_dir.native());
}

class AskpassDirectorMonitor : public sigc::trackable {
    Askpass::Model<UiManager> &m_model;
    Glib::RefPtr<Gio::File> m_askpass_directory;
    Glib::RefPtr<Gio::FileMonitor> m_file_monitor;
    bool m_directory_enumerated {false};
    bool m_idle_signal_installed {false};

    void enumerate_directory() {
        auto enumerator = m_askpass_directory->enumerate_children(
            G_FILE_ATTRIBUTE_STANDARD_NAME "," G_FILE_ATTRIBUTE_STANDARD_TYPE);
        for (auto file = enumerator->next_file(); file; file = enumerator->next_file()) {
            if (file->get_file_type() == Gio::FileType::REGULAR && file->get_name().starts_with("ask.")) {
                m_model.on_file_created(enumerator->get_child(file));
            }
        }

        enqueue_events_ended_signal();
        m_directory_enumerated = true;
    }

    void events_ended_signal() {
        m_model.on_file_events_ended();
        m_idle_signal_installed = false;
    }

    void enqueue_events_ended_signal() {
        if (!m_idle_signal_installed) {
            Glib::signal_idle().connect_once(sigc::mem_fun(*this, &AskpassDirectorMonitor::events_ended_signal));
            m_idle_signal_installed = true;
        }
    }

    void on_signal_changed(const Glib::RefPtr<Gio::File> &file, const Glib::RefPtr<Gio::File> &,
        Gio::FileMonitor::Event event) {
        // Gio::FileMonitor behaves weirdly when the directory does not exists.
        // We can't know if this event is caused by the ask-password directory or an ask-password file in the directory
        assert(file);
        auto file_name = file->get_basename();
        if (file_name == "ask-password") {
            if (!m_directory_enumerated && event == Gio::FileMonitor::Event::CREATED) {
                enumerate_directory();
            } else if (event == Gio::FileMonitor::Event::DELETED) {
                m_directory_enumerated = false;
            }
        } else if (file_name.starts_with("ask.")) {
            if (event == Gio::FileMonitor::Event::CREATED) {
                m_model.on_file_created(file);
                enqueue_events_ended_signal();
            } else if (event == Gio::FileMonitor::Event::DELETED) {
                m_model.on_file_deleted(file);
            }
        }
    }

public:
    AskpassDirectorMonitor(Askpass::Model<UiManager> &model) :
            m_model(model), m_askpass_directory(get_askpass_directory_gio_file()),
            m_file_monitor(m_askpass_directory->monitor_directory()) {
        m_file_monitor->signal_changed().connect(sigc::mem_fun(*this, &AskpassDirectorMonitor::on_signal_changed));
        enumerate_directory();
    }
};

int main(int argc, char **argv) {
    UiManager ui_manager {};
    Askpass::Model model {ui_manager};
    AskpassDirectorMonitor monitor {model};

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
