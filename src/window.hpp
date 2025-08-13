#ifndef _WINDOW_HPP
#define _WINDOW_HPP

#include "model.hpp"

#include <gtkmm.h>

class Window final : public Gtk::ApplicationWindow {
private:
    Gtk::Label m_label;
    Gtk::PasswordEntry m_password_entry;
    Gtk::Button m_cancel_button;
    Gtk::Button m_ok_button;
    Gtk::Grid m_grid;

    sigc::signal<on_succeeded_func_t> m_signal_succeeded;
    sigc::signal<on_failure_func_t> m_signal_failure;

    bool m_finished = false;

public:
    Window(const Glib::ustring& label_text);
    Window(AskpassModel& model);
    ~Window();

    sigc::signal<on_succeeded_func_t> signal_succeeded() {
        return m_signal_succeeded;
    }

    sigc::signal<on_failure_func_t> signal_failure() {
        return m_signal_failure;
    }

private:
    bool on_close_request() final;
    void on_ok_button_clicked();
    void on_cancel_butto_clicked();

    void emit_succeeded();
    void emit_failure();
};

static_assert(WindowInterface<Window>);

#endif