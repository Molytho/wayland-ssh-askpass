#include "window.hpp"

#include <string_view>

#include <gtk4-layer-shell.h>

namespace {
    constexpr std::string_view TITLE = "Askpass";
    constexpr int TARGET_WIDTH = 250;
    constexpr int TARGET_HEIGHT = 300;

    constexpr const char *LAYER_NAMESPACE = "Password Dialog";
    constexpr GtkLayerShellLayer LAYER = GTK_LAYER_SHELL_LAYER_OVERLAY;
    constexpr GtkLayerShellKeyboardMode KEYBOARD_MODE = GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE;

    void setup_gtk_layer_shell(Gtk::Window& window) {
        auto gobj = window.gobj();
        gtk_layer_init_for_window(gobj);
        gtk_layer_set_namespace(gobj, LAYER_NAMESPACE);
        gtk_layer_set_layer(gobj, LAYER);
        gtk_layer_set_keyboard_mode(gobj, KEYBOARD_MODE);
    }
}

Window::Window(const Glib::ustring& label_text) {
    m_label.set_label(label_text);
    m_label.set_selectable(false);
    m_label.set_expand(true);
    m_label.set_margin(15);

    m_password_entry.set_show_peek_icon(true);
    m_password_entry.set_enable_undo(false);
    m_password_entry.property_activates_default().set_value(true);

    m_cancel_button.set_label("Cancel");
    m_cancel_button.signal_clicked().connect(sigc::mem_fun(*this, &Gtk::Window::close));

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

    set_title(std::string(TITLE));
    set_default_size(TARGET_WIDTH, TARGET_HEIGHT);
    set_default_widget(m_ok_button);

    setup_gtk_layer_shell(*this);
}

Window::Window(AskpassModel& model) : Window(std::string(model.get_message())) {
    model.register_window(*this);
}

Window::~Window() = default;

bool Window::on_close_request() {
    emit_failure();
    return false;
}

void Window::on_ok_button_clicked() {
    emit_succeeded();
    close();
}


void Window::emit_succeeded() {
    if (!m_finished) {
        auto password_entry_editable_obj = m_password_entry.Gtk::Editable::gobj();
        std::string_view input = gtk_editable_get_text(password_entry_editable_obj);
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