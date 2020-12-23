
#ifndef OC_FILEREADER
#define OC_FILEREADER

#include <array>
#include <cstdio>
#include <string>
#include <unordered_map>

namespace oc {

// Everything that writes to hints.attributes_ is performing a copy.  Switch to unique_ptrs?
class FileReaderHints {
    public:
        FileReaderHints() = default;
        explicit FileReaderHints(const std::string& path);

        bool magic_number(const std::string& bytes) const noexcept;
        template<std::size_t N> bool magic_number(const std::array<int, N>& bytes) const noexcept;

        const std::array<int, 8>& first_bytes() const noexcept {return first_bytes_;}
        void first_bytes(const std::array<int, 8>& bytes){first_bytes_ = bytes;}

        uintmax_t size() const noexcept {return size_;}
        void size(std::size_t size){size_ = size;}

        const std::string& filename() const noexcept {return filename_;}
        void filename(const std::string& filename){filename_ = filename;}

        const std::string& format() const noexcept {return format_;}
        void format(const std::string& format){format_ = format;}

        auto& attributes(){return attributes_;}
    private:
        std::string filename_{};
        std::array<int, 8> first_bytes_{{0, 0, 0, 0, 0, 0, 0, 0}};
        uintmax_t size_{0};
        std::string format_{"unknown"};
        std::unordered_map<std::string, std::string> attributes_{};
};
class FileReader {
    public:
        enum validity {
            invalid, possibly_valid, valid
        };
        FileReader() = default;
        virtual ~FileReader() = default;

        virtual validity is_valid_file(const std::string& file, FileReaderHints& hints);
        validity is_valid_file(const std::string& file){ FileReaderHints hints{file}; return is_valid_file(file, hints); }

    private:
};

} // namespace oc

#endif // OC_FILEREADER

