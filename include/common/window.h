#ifndef _WINDOW_H
#define _WINDOW_H

#include <gtkmm.h>

#include "concepts.h"
#include "exit_codes.h"

namespace Askpass {
    class Window final : public Gtk::ApplicationWindow {
    private:
        using Base = Gtk::ApplicationWindow;

        Gtk::Label m_label;
        Gtk::PasswordEntry m_password_entry;
        Gtk::Button m_cancel_button;
        Gtk::Button m_ok_button;
        Gtk::Grid m_grid;

        sigc::signal<on_succeeded_func_t> m_signal_succeeded;
        sigc::signal<on_failure_func_t> m_signal_failure;

        bool m_finished = false;

    public:
        Window(const Glib::ustring &label_text);

        Window(std::string_view string_view);

        Window(WindowModelInterface<Window> auto &model) : Window(model.message()) {
            model.register_window(*this);
        }

        ~Window();

        sigc::signal<on_succeeded_func_t> signal_succeeded() { return m_signal_succeeded; }

        sigc::signal<on_failure_func_t> signal_failure() { return m_signal_failure; }

    private:
        void on_realize() final;
        void on_ok_button_clicked();
        void on_cancle_button_clicked();
        bool on_key_pressed(guint keyval, guint, Gdk::ModifierType state);

        void emit_succeeded();
        void emit_failure();

        void setup_controllers();
    };

    static_assert(WindowInterface<Window>);

    void make_and_run_window(std::string_view app_id, WindowModelInterface<Window> auto &model) {
        static constexpr const char AllowedBackends[] = "wayland,x11";
        gdk_set_allowed_backends(AllowedBackends);
        auto app = Gtk::Application::create(std::string(app_id));
        if (app->make_window_and_run<Askpass::Window>(0, nullptr, model)) {
            return exit(ExitCode::Unknown);
        }
    }
} // namespace Askpass

#endif