#ifndef CONCEPTS_H
#define CONCEPTS_H

#include <concepts>

#include <sigc++/signal.h>

namespace Askpass {
    using on_succeeded_func_t = void(const std::string_view &input);
    using on_failure_func_t   = void();

    template<class T>
    concept WindowInterface = requires(T &obj) {
        { obj.signal_succeeded() } -> std::same_as<sigc::signal<on_succeeded_func_t>>;
        { obj.signal_failure() } -> std::same_as<sigc::signal<on_failure_func_t>>;
    };
} // namespace Askpass

#endif