#include "systemd-askpass-context.h"

#include <span>

#include <boost/program_options.hpp>

#include <sys/socket.h>
#include <sys/un.h>

#include "macros.h"

namespace po = boost::program_options;

namespace {
    constexpr char OptionMessage[]  = "Ask.Message";
    constexpr char OptionPID[]      = "Ask.PID";
    constexpr char OptionSocket[]   = "Ask.Socket";
    constexpr char OptionNotAfter[] = "Ask.NotAfter";

    po::options_description create_option_description() {
        // clang-format off
        po::options_description desc {};
        desc.add_options()
            (OptionMessage , po::value<std::string>()->default_value("No message"), "Message to show")
            (OptionPID     , po::value<int>()->required(), "PID of sender")
            (OptionSocket  , po::value<std::string>()->required(), "The answer socket")
            (OptionNotAfter, po::value<time_t>()->default_value(0), "The timeout");;
        return desc;
        // clang-format on
    }

    template<size_t N, size_t M>
    void span_copy(std::span<char, N> dest, std::span<const char, M> src) {
        if (dest.size() < src.size()) {
            throw std::invalid_argument("dest is too small for src");
        }
        std::memcpy(dest.data(), src.data(), src.size());
    }

    wrapper::unique_fd create_answer_socket(std::string_view path) {
        const sockaddr_un addr = [&]() {
            sockaddr_un res {};
            res.sun_family = AF_UNIX;
            auto path_span = std::span(res.sun_path);
            // We need the last character as null-terminator
            span_copy(path_span.subspan<0, path_span.size() - 1>(), std::span(path));
            return res;
        }();

        wrapper::unique_fd s {socket(AF_UNIX, SOCK_DGRAM, 0)};
        throw_system_error_if(s.get() < 0);
        if (connect(s.get(), reinterpret_cast<const sockaddr *>(&addr), sizeof(addr)) < 0) {
            throw std::system_error(errno, std::system_category());
        }
        return s;
    }
} // namespace

namespace Askpass {
    SystemdAskpassContext::SystemdAskpassContext(
        std::string m_message, int m_pid, wrapper::unique_fd m_answer_socket, time_t m_timeout) :
            m_message(std::move(m_message)), m_pid(m_pid),
            m_answer_socket(std::move(m_answer_socket)), m_timeout(m_timeout) {}

    std::unique_ptr<SystemdAskpassContext> SystemdAskpassContext::from_askpass_file(
        std::basic_istream<char> &askpass_file) {
        static const auto options = create_option_description();

        po::variables_map vm;
        po::store(po::parse_config_file(askpass_file, options, true), vm);
        po::notify(vm);

        return std::make_unique<SystemdAskpassContext>(std::move(vm.at(OptionMessage).as<std::string>()),
            vm.at(OptionPID).as<int>(),
            create_answer_socket(vm.at(OptionSocket).as<std::string>()),
            vm.at(OptionNotAfter).as<time_t>());
    }
} // namespace Askpass
