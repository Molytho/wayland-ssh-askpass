#include "window.h"

#include <iostream>
#include <string_view>

#include <gdk/gdkkeysyms.h>

#include "exit_codes.h"

#ifdef GDK_WINDOWING_WAYLAND
# include <gdk/wayland/gdkwayland.h>

void platform_setup_wayland(Gtk::Window &window);

bool is_wayland_display(Gdk::Display *display) {
    return GDK_IS_WAYLAND_DISPLAY(display->gobj());
}
#else
[[maybe_unused]] void platform_setup_wayland(Gtk::Window &) {}

bool is_wayland_display(Gdk::Display *) {
    return false;
}
#endif

#ifdef GDK_WINDOWING_X11
# include <gdk/x11/gdkx.h>

void platform_setup_x11(Gtk::Window &window);

bool is_x11_display(Gdk::Display *display) {
    return GDK_IS_X11_DISPLAY(display->gobj());
}
#else
[[maybe_unused]] void platform_setup_x11(Gtk::Window &) {}

bool is_x11_display(Gdk::Display *) {
    return false;
}
#endif

namespace {
    constexpr std::string_view Title = "Askpass";

    void platform_setup(Askpass::Window &window) {
        if (is_wayland_display(window.get_display().get())) {
            platform_setup_wayland(window);
        } else if (is_x11_display(window.get_display().get())) {
            platform_setup_x11(window);
        } else {
            std::cerr << "Invalid gdk platform\n";
            exit(Askpass::ExitCode::InvalidPlatform);
        }
    }
} // namespace

namespace Askpass {
    Window::Window(const Glib::ustring &label_text) {
        m_label.set_label(label_text);
        m_label.set_selectable(false);
        m_label.set_expand(true);
        m_label.set_margin(15);

        m_password_entry.set_show_peek_icon(true);
        m_password_entry.set_enable_undo(false);
        m_password_entry.property_activates_default().set_value(true);

        m_cancel_button.set_label("Cancel");
        m_cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &Window::on_cancle_button_clicked));

        m_ok_button.set_label("Ok");
        m_ok_button.signal_clicked().connect(sigc::mem_fun(*this, &Window::on_ok_button_clicked));

        m_grid.set_row_homogeneous(false);
        m_grid.set_column_homogeneous(true);
        m_grid.set_margin(5);
        m_grid.set_column_spacing(5);
        m_grid.set_row_spacing(5);

        m_grid.attach(m_label, 0, 0, 2, 1);
        m_grid.attach(m_password_entry, 0, 1, 2, 1);
        m_grid.attach(m_cancel_button, 0, 2);
        m_grid.attach(m_ok_button, 1, 2);
        set_child(m_grid);

        set_title(std::string(Title));
        set_default_widget(m_ok_button);
        set_decorated(false);
        set_deletable(false);
        set_resizable(false);

        setup_controllers();
    }

    Window::Window(std::string_view string_view) :
            Window(Glib::ustring(string_view.data(), string_view.size())) {}

    Window::~Window() = default;

    void Window::on_realize() {
        Base::on_realize();
        platform_setup(*this);
    }

    void Window::on_ok_button_clicked() {
        emit_succeeded();
        close();
    }

    void Window::on_cancle_button_clicked() {
        emit_failure();
        close();
    }

    bool Window::on_key_pressed(guint keyval, guint, Gdk::ModifierType) {
        if (keyval == GDK_KEY_Escape) {
            on_cancle_button_clicked();
            return true;
        }
        return false;
    }

    void Window::emit_succeeded() {
        if (!m_finished) {
            auto password_entry_editable_obj = m_password_entry.Gtk::Editable::gobj();
            std::string_view input           = gtk_editable_get_text(password_entry_editable_obj);
            m_signal_succeeded.emit(input);
            m_finished = true;
        }
    }

    void Window::emit_failure() {
        if (!m_finished) {
            m_signal_failure.emit();
            m_finished = true;
        }
    }

    void Window::setup_controllers() {
        add_controller([&]() {
            auto key_controller = Gtk::EventControllerKey::create();
            key_controller->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
            key_controller->signal_key_pressed().connect(sigc::mem_fun(*this, &Window::on_key_pressed), true);
            return key_controller;
        }());
    }

} // namespace Askpass