#ifndef MODEL_H
#define MODEL_H

#include <cassert>
#include <optional>
#include <unordered_set>

#include <giomm.h>
#include <gtkmm.h>

#include "window-model.h"

namespace Askpass::detail {
    struct AskpassFileImpl {
        Glib::RefPtr<Gio::File> file;

        AskpassFileImpl(Glib::RefPtr<Gio::File> file);
    };

    bool operator==(const AskpassFileImpl &lhs, const AskpassFileImpl &rhs) noexcept;

    std::unique_ptr<Askpass::SystemdAskpassContext> read_askpass_file(const AskpassFileImpl &file);
} // namespace Askpass::detail

template<>
struct std::hash<Askpass::detail::AskpassFileImpl> {
    guint operator()(const Askpass::detail::AskpassFileImpl &file) const noexcept;
};

namespace Askpass {
    template<class T>
    concept AskpassFileInterface = requires(T &obj) {
        { read_askpass_file(obj) } -> std::same_as<std::unique_ptr<SystemdAskpassContext>>;
    };

    template<class T>
    concept UiInterface = requires(T &obj, WindowModel &window_model) {
        obj.spawn_window(window_model);
        { obj.signal_window_closed() } -> std::same_as<sigc::signal<void(void)>>;
    };

    template<class T>
    class FileStorage {
        std::unordered_set<T> m_storage;

    public:
        void add_file(const T &file) { m_storage.emplace(file); }

        void add_file(T &&file) { m_storage.emplace(std::move(file)); }

        void remove_file(const T &file) { m_storage.erase(file); }

        T dequeue_file() {
            if (empty()) {
                throw std::runtime_error("dequeue while storage is empty");
            }
            auto begin = m_storage.begin();
            auto node  = m_storage.extract(begin);
            return std::move(node.value());
        }

        bool empty() const noexcept { return m_storage.empty(); }
    };

    using AskpassFile = detail::AskpassFileImpl;
    static_assert(AskpassFileInterface<AskpassFile>);

    template<UiInterface T>
    class Model {
        FileStorage<AskpassFile> m_current_askpass_files;
        std::optional<WindowModel> m_run_window_model;
        T &m_ui_manager;

        void check_spawn_window() {
            if (!m_run_window_model.has_value() && !m_current_askpass_files.empty()) {
                AskpassFileInterface auto file = m_current_askpass_files.dequeue_file();
                m_run_window_model.emplace(read_askpass_file(file));
                m_ui_manager.spawn_window(*m_run_window_model);
            }
        }

        void on_window_closed() {
            m_run_window_model.reset();
            check_spawn_window();
        }

    public:
        Model(T &ui_manager) : m_ui_manager(ui_manager) {
            m_ui_manager.signal_window_closed().connect([this]() { this->on_window_closed(); });
        }

        void on_file_created(AskpassFile file) {
            m_current_askpass_files.add_file(std::move(file));
        }

        void on_file_deleted(AskpassFile file) { m_current_askpass_files.remove_file(file); }

        void on_file_events_ended() { check_spawn_window(); }
    };
} // namespace Askpass

#endif