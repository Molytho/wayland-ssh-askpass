#include <gtkmm.h>

#ifdef GDK_WINDOWING_X11

# include <array>
# include <iostream>
# include <span>

# include <X11/Xatom.h>
# include <gdk/x11/gdkx.h>

namespace {
    // X11 stuff
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

    void set_window_state(Display *xdisplay, Window xwindow, Atom key, std::span<const Atom> values) {
        set_atom(xdisplay, xwindow, key, values);
    }

    void grab_keyboard(Display *xdisplay, Window xwindow) {
        if (auto res = XGrabKeyboard(xdisplay, xwindow, True, GrabModeAsync, GrabModeAsync, CurrentTime);
            res != GrabSuccess) {
            std::cerr << "Failed to grab keyboard focus";
            exit(EXIT_FAILURE);
        }
    }

    void grab_pointer(Display *xdisplay, Window xwindow) {
        // Everything
        constexpr int GrabMask
            = ButtonPressMask
              | ButtonReleaseMask
              | EnterWindowMask
              | LeaveWindowMask
              | PointerMotionMask
              | PointerMotionHintMask
              | Button1MotionMask
              | Button2MotionMask
              | Button3MotionMask
              | Button4MotionMask
              | Button5MotionMask
              | ButtonMotionMask
              | KeymapStateMask;
        if (auto res = XGrabPointer(xdisplay, xwindow, True, GrabMask, GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
            res != GrabSuccess) {
            std::cerr << "Failed to grab keyboard focus";
            exit(EXIT_FAILURE);
        }
    }

    gboolean on_xevent(GdkX11Display *display, gpointer xevent, gpointer user_data) {
        if (auto event = static_cast<XEvent *>(xevent); event->type == Expose) {
            auto surface = static_cast<GdkSurface *>(user_data);
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            auto xdisplay = gdk_x11_display_get_xdisplay(display);
            auto xwindow  = gdk_x11_surface_get_xid(surface);
# pragma GCC diagnostic pop
            grab_keyboard(xdisplay, xwindow);
            grab_pointer(xdisplay, xwindow);
        }
        return false;
    }
} // namespace

void platform_setup_x11(Gtk::Window &window) {
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    auto xdisplay    = gdk_x11_display_get_xdisplay(window.get_display()->gobj());
    auto xrootwindow = gdk_x11_display_get_xrootwindow(window.get_display()->gobj());
    auto xwindow     = gdk_x11_surface_get_xid(window.get_surface()->gobj());

    const auto atoms = get_atoms(xdisplay);

    XSetTransientForHint(xdisplay, xwindow, xrootwindow);
    const std::array DesiredWindowStates {atoms[ATOMS_WM_STATE_ABOVE],
        atoms[ATOMS_WM_STATE_STICKY],
        atoms[ATOMS_WM_STATE_MODAL]};
    set_window_state(xdisplay, xwindow, atoms[ATOMS_WM_STATE], DesiredWindowStates);

    auto x11_surface = GDK_X11_SURFACE(window.get_surface()->gobj());
    gdk_x11_surface_move_to_current_desktop(x11_surface);

    GdkX11Display *x11_display = GDK_X11_DISPLAY(window.get_display()->gobj());
    g_signal_connect_object(x11_display, "xevent", G_CALLBACK(&on_xevent), window.get_surface()->gobj(), G_CONNECT_DEFAULT);
# pragma GCC diagnostic pop
}

#endif