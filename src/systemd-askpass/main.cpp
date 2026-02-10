#include <iostream>
#include <string>

#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char **argv) {
    // Declare the supported options.
    po::options_description desc("Allowed options");
    desc.add_options()
        ("Ask.Message", po::value<std::string>(), "Message to show")
        ("Ask.PID", po::value<int>(), "PID of sender")
        ("Ask.Echo", po::value<int>(), "Whether the input should be echoed")
        ("Ask.Socket", po::value<std::string>(), "The answer socket");

    po::variables_map vm;
    if (argc > 1) {
        po::store(po::parse_config_file(argv[1], desc, true), vm);
    }
    po::notify(vm);

    if (vm.count("Ask.Message")) {
        std::cout << vm["Ask.Message"].as<std::string>() << "\n";
    }

    return 0;
}
