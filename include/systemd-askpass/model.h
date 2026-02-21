#ifndef MODEL_H
#define MODEL_H

#include <cassert>
#include <csignal>
#include <iostream>
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
    concept UiInterface = requires(T &obj, WindowModel &window_model, unsigned int milliseconds,
        const sigc::slot<bool()> &timeout_func) {
        obj.spawn_window(window_model);
        obj.close_window();
        { obj.set_timeout(milliseconds, timeout_func) } -> std::same_as<sigc::connection>;
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
    class Model : public sigc::trackable {
        struct run_context {
            WindowModel window_model;
            sigc::scoped_connection timeout_slot;
        };

        T &m_ui_manager;
        FileStorage<AskpassFile> m_current_askpass_files;
        std::optional<run_context> m_run_context;

        static unsigned int calculate_timeout(time_t microseconds) {
            if (microseconds == 0) {
                return 0;
            }
            timespec buffer {};
            if (clock_gettime(CLOCK_MONOTONIC, &buffer) < 0) {
                std::abort();
            }
            time_t current_milli_sec = buffer.tv_sec * 1000 + buffer.tv_nsec / 1000000;
            return std::max<unsigned int>(0, (microseconds / 1000) - current_milli_sec);
        }

        std::optional<WindowModel> make_next_window_model() {
            while (!m_current_askpass_files.empty()) {
                AskpassFileInterface auto file = m_current_askpass_files.dequeue_file();
                std::unique_ptr<SystemdAskpassContext> context;
                try {
                    context = read_askpass_file(file);
                } catch (const std::runtime_error &ex) {
                    std::cerr << "Reading Askpass file failed:\n" << ex.what() << '\n';
                    continue;
                }
                if (calculate_timeout(context->timeout()) <= 0) {
                    std::cout << "Askpass request already timed out\n";
                    continue;
                }
                if (kill(context->pid(), 0) < 0 && errno == ESRCH) {
                    std::cout << "Askpass process already disappeared\n";
                    continue;
                }
                return WindowModel(std::move(context));
            }
            return {};
        }

        void check_spawn_window() {
            if (std::optional<WindowModel> window_model;
                !m_run_context.has_value() && (window_model = make_next_window_model())) {
                m_ui_manager.spawn_window(*window_model);
                auto timeout_slot = m_ui_manager.set_timeout(calculate_timeout(window_model->timeout()),
                    sigc::mem_fun(*this, &Model::on_timeout));
                m_run_context.emplace(*std::move(window_model), timeout_slot);
            }
        }

        bool on_timeout() {
            m_ui_manager.close_window();
            return false;
        }

        void on_window_closed() {
            m_run_context.reset();
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