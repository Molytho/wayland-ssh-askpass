#ifndef _WINDOW_H
#define _WINDOW_H

#include <gtkmm.h>

namespace Askpass {
    using on_succeeded_func_t = void(const std::string_view &input);
    using on_failure_func_t   = void();

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

        Window(auto &model) : Window(model.get_message()) {
            model.register_window(*this);
        }

        ~Window();

        sigc::signal<on_succeeded_func_t> signal_succeeded() { return m_signal_succeeded; }

        sigc::signal<on_failure_func_t> signal_failure() { return m_signal_failure; }

    private:
        void on_realize() final;
        bool on_close_request() final;
        void on_ok_button_clicked();
        bool on_key_pressed(guint keyval, guint, Gdk::ModifierType state);

        void emit_succeeded();
        void emit_failure();

        void setup_controllers();
    };
} // namespace Askpass

#endif