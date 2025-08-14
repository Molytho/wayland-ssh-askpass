#include "window.hpp"

#include <string_view>

// Wayland stuff
#ifdef GDK_WINDOWING_WAYLAND
# include <gdk/wayland/gdkwayland.h>
# include <gtk4-layer-shell.h>
#endif

namespace {
    constexpr std::string_view TITLE = "Askpass";
    constexpr int TARGET_WIDTH       = 250;
    constexpr int TARGET_HEIGHT      = 300;

// Wayland stuff
#ifdef GDK_WINDOWING_WAYLAND
    constexpr const char LAYER_NAMESPACE[]            = "Password Dialog";
    constexpr GtkLayerShellLayer LAYER                = GTK_LAYER_SHELL_LAYER_OVERLAY;
    constexpr GtkLayerShellKeyboardMode KEYBOARD_MODE = GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE;

    void setup_gtk_layer_shell(Gtk::Window &window) {
        auto gobj = window.gobj();
        gtk_layer_init_for_window(gobj);
        gtk_layer_set_namespace(gobj, LAYER_NAMESPACE);
        gtk_layer_set_layer(gobj, LAYER);
        gtk_layer_set_keyboard_mode(gobj, KEYBOARD_MODE);
    }

    void platform_setup_wayland(Askpass::Window &window) {
        if (gtk_layer_is_supported()) {
            setup_gtk_layer_shell(window);
        } else {
            std::cerr << "This application only supports wlr_layer_shell\n";
            exit(EXIT_FAILURE);
        }
    }
#endif

    void platform_setup(Askpass::Window &window) {
        auto display = window.get_display()->gobj();
#ifdef GDK_WINDOWING_WAYLAND
        if (GDK_IS_WAYLAND_DISPLAY(display)) {
            platform_setup_wayland(window);
        } else
#endif
        {
            std::cerr << "Invalid gdk platform\n";
            abort();
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
    }

    Window::Window(Model &model) : Window(std::string(model.get_message())) {
        model.register_window(*this);
    }

    Window::~Window() = default;

    void Window::on_realize() {
        Base::on_realize();
        platform_setup(*this);
    }

    bool Window::on_close_request() {
        emit_failure();
        return Base::on_close_request();
    }

    void Window::on_ok_button_clicked() {
        emit_succeeded();
        close();
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

} // namespace Askpass