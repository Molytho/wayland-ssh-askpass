#include "window.hpp"

#include <array>
#include <span>
#include <string_view>

#include <gdk/gdkkeysyms.h>

// X11 Stuff
#ifdef GDK_WINDOWING_X11
# include <X11/Xatom.h>
# include <gdk/x11/gdkx.h>
#endif

// Wayland stuff
#ifdef GDK_WINDOWING_WAYLAND
# include <gdk/wayland/gdkwayland.h>
# include <gtk4-layer-shell.h>
#endif

namespace {
    constexpr std::string_view TITLE = "Askpass";

// X11 stuff
#ifdef GDK_WINDOWING_X11
    enum Atoms {
        ATOMS_WM_WINDOW_TYPE,
        ATOMS_WM_WINDOW_TYPE_DIALOG,
        ATOMS_WM_STATE,
        ATOMS_WM_STATE_STICKY,
        ATOMS_WM_STATE_ABOVE,
        ATOMS_WM_STATE_MODAL,
        ATOMS_MAX
    };

    constexpr std::array NEEDED_ATOMS {"_NET_WM_WINDOW_TYPE",
        "_NET_WM_WINDOW_TYPE_DIALOG",
        "_NET_WM_STATE",
        "_NET_WM_STATE_STICKY",
        "_NET_WM_STATE_ABOVE",
        "_NET_WM_STATE_MODAL"};
    static_assert(ATOMS_MAX == NEEDED_ATOMS.size());

    std::array<Atom, ATOMS_MAX> get_atoms(Display *xdisplay) {
        std::array<Atom, ATOMS_MAX> buffer {};
        // XInternAtoms is broken and requested a char**
        XInternAtoms(xdisplay, const_cast<char **>(NEEDED_ATOMS.data()), ATOMS_MAX, false, buffer.data());
        return buffer;
    }

    void set_atom(Display *xdisplay, Window xwindow, Atom key, std::span<const Atom> values) {
        XChangeProperty(xdisplay,
            xwindow,
            key,
            XA_ATOM,
            32,
            PropModeReplace,
            reinterpret_cast<const unsigned char *>(values.data()),
            values.size());
    }

    void set_atom(Display *xdisplay, Window xwindow, Atom key, Atom value) {
        std::array values {value};
        set_atom(xdisplay, xwindow, key, values);
    }

    void set_window_state(Display *xdisplay, Window xwindow, Atom key, std::span<const Atom> values) {
        set_atom(xdisplay, xwindow, key, values);

        /*XClientMessageEvent update_event {
            .type         = ClientMessage,
            .serial       = 0,
            .send_event   = false,
            .display      = nullptr,
            .window       = xwindow,
            .message_type = key,
            .format       = 32,
            .data         = {},
        };
        update_event.data.l[0] = 1; // Add/set property
        update_event.data.l[1] = static_cast<long>(atoms[ATOMS_WM_STATE_ABOVE]);
        update_event.data.l[2] = static_cast<long>(atoms[ATOMS_WM_STATE_STICKY]);
        update_event.data.l[3] = 1; // Normal application
        if (!XSendEvent(xdisplay, xrootwindow, false, 0, reinterpret_cast<XEvent *>(&update_event))) {
            std::cerr << "Sending XEvent failed\n";
            abort();
        }*/
    }

    void set_override_redirect(Display *xdisplay, Window xwindow) {
        XSetWindowAttributes attrs {};
        attrs.override_redirect = True;
        XChangeWindowAttributes(xdisplay, xwindow, CWOverrideRedirect, &attrs);
    }

    void set_position(Display *xdisplay, Gtk::Window &window, Window xwindow) {
        auto default_screen = DefaultScreen(xdisplay);
        auto display_width  = DisplayWidth(xdisplay, default_screen);
        auto display_height = DisplayHeight(xdisplay, default_screen);

        auto width_meassurement = window.measure(Gtk::Orientation::HORIZONTAL);
        auto height_meassurement = window.measure(Gtk::Orientation::VERTICAL);

        XWindowChanges changes {.x = (display_width - width_meassurement.sizes.natural) / 2,
            .y                     = (display_height - height_meassurement.sizes.natural) / 2,
            .width                 = {},
            .height                = {},
            .border_width          = {},
            .sibling               = {},
            .stack_mode            = Above};
        XConfigureWindow(xdisplay, xwindow, CWX | CWY | CWStackMode, &changes);
    }

    void grab_keyboard(Display *xdisplay, Window xwindow) {
        if (!XGrabKeyboard(xdisplay, xwindow, True, GrabModeAsync, GrabModeAsync, CurrentTime)) {
            std::cerr << "Failed to grab keyboard focus";
            exit(EXIT_FAILURE);
        }
    }

    void platform_setup_x11(Gtk::Window &window) {
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        auto xdisplay = gdk_x11_display_get_xdisplay(window.get_display()->gobj());
        // auto xrootwindow = gdk_x11_display_get_xrootwindow(window.get_display()->gobj());
        auto xwindow = gdk_x11_surface_get_xid(window.get_surface()->gobj());
# pragma GCC diagnostic pop

        const auto atoms = get_atoms(xdisplay);
        set_override_redirect(xdisplay, xwindow);
        set_position(xdisplay, window, xwindow);
        // Window type
        set_atom(xdisplay, xwindow, atoms[ATOMS_WM_WINDOW_TYPE], atoms[ATOMS_WM_WINDOW_TYPE_DIALOG]);
        // Window state
        const std::array DesiredWindowStates {atoms[ATOMS_WM_STATE_ABOVE],
            atoms[ATOMS_WM_STATE_STICKY],
            atoms[ATOMS_WM_STATE_MODAL]};
        set_window_state(xdisplay, xwindow, atoms[ATOMS_WM_STATE], DesiredWindowStates);

        grab_keyboard(xdisplay, xwindow);

        XFlush(xdisplay);
    }
#endif

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
#ifdef GDK_WINDOWING_X11
            if (GDK_IS_X11_DISPLAY(display)) {
            platform_setup_x11(window);
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
        set_default_widget(m_ok_button);

        add_controller([&]() {
            auto key_controller = Gtk::EventControllerKey::create();
            key_controller->set_propagation_phase(Gtk::PropagationPhase::BUBBLE);
            key_controller->signal_key_pressed().connect(sigc::mem_fun(*this, &Window::on_key_pressed), true);
            return key_controller;
        }());
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

    bool Window::on_key_pressed(guint keyval, guint, Gdk::ModifierType) {
        if (keyval == GDK_KEY_Escape) {
            close();
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

} // namespace Askpass