#include "model.h"

#include <cassert>
#include <sstream>

namespace {
    std::string read_gio_file(Gio::File &file) {
        auto input_stream = file.read();
        auto size_hint    = file.query_info(G_FILE_ATTRIBUTE_STANDARD_SIZE)->get_size();

        std::string buffer {};
        buffer.resize(size_hint + 1);

        gssize bytes_read = input_stream->read(buffer.data(), buffer.size());
        if (bytes_read < 0) {
            std::abort();
        }

        // File size change. Unlucky :(
        constexpr size_t SizeIncreaseOnReadNotEOF = 256;
        while (gsize(bytes_read) == buffer.size()) {
            buffer.resize(buffer.size() + SizeIncreaseOnReadNotEOF);
            gssize read = input_stream->read(buffer.data() + bytes_read, SizeIncreaseOnReadNotEOF);
            if (read < 0) {
                std::abort();
            }
            bytes_read += read;
        }

        buffer.resize(bytes_read);
        return buffer;
    }
} // namespace

namespace Askpass::detail {
    bool operator==(const AskpassFileImpl &lhs, const AskpassFileImpl &rhs) noexcept {
        return lhs.file->equal(rhs.file);
    }

    std::unique_ptr<Askpass::SystemdAskpassContext> read_askpass_file(const AskpassFileImpl &file) {
        std::istringstream istream {read_gio_file(*file.file)};
        return Askpass::SystemdAskpassContext::from_askpass_file(istream);
    }
} // namespace Askpass::detail

guint std::hash<Askpass::detail::AskpassFileImpl>::operator()(const Askpass::detail::AskpassFileImpl &file) const noexcept {
    return file.file->hash();
};