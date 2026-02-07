#include <gtkmm.h>

#ifdef GDK_WINDOWING_WAYLAND

# include <iostream>

# include <gdk/wayland/gdkwayland.h>
# include <gtk4-layer-shell.h>

namespace {
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
} // namespace

void platform_setup_wayland(Gtk::Window &window) {
    if (gtk_layer_is_supported()) {
        setup_gtk_layer_shell(window);
    } else {
        std::cerr << "This application only supports wlr_layer_shell\n";
        exit(EXIT_FAILURE);
    }
}

#endif
