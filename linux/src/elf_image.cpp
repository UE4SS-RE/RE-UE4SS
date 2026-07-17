#include "ue4ss/elf_image.hpp"

#include "ue4ss/sha256.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <elf.h>
#include <fcntl.h>
#include <limits>
#include <link.h>
#include <stdexcept>
#include <string_view>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

namespace
{
    [[nodiscard]] constexpr std::size_t align4(std::size_t value)
    {
        return (value + 3u) & ~std::size_t{3u};
    }

    [[nodiscard]] bool range_fits(std::size_t offset, std::size_t length, std::size_t total)
    {
        return offset <= total && length <= total - offset;
    }

    [[nodiscard]] std::string bytes_to_hex(const std::uint8_t* bytes, std::size_t length)
    {
        constexpr char k_hex[] = "0123456789abcdef";
        std::string output(length * 2u, '\0');
        for (std::size_t index = 0; index < length; ++index)
        {
            output[index * 2u] = k_hex[bytes[index] >> 4u];
            output[index * 2u + 1u] = k_hex[bytes[index] & 0x0fu];
        }
        return output;
    }

    [[nodiscard]] std::string find_build_id(const std::byte* data, std::size_t size)
    {
        std::size_t cursor = 0;
        while (range_fits(cursor, sizeof(Elf64_Nhdr), size))
        {
            Elf64_Nhdr header{};
            std::memcpy(&header, data + cursor, sizeof(header));
            cursor += sizeof(header);
            const std::size_t name_size = header.n_namesz;
            const std::size_t description_size = header.n_descsz;
            if (!range_fits(cursor, align4(name_size), size))
            {
                break;
            }
            const auto* name = reinterpret_cast<const char*>(data + cursor);
            cursor += align4(name_size);
            if (!range_fits(cursor, align4(description_size), size))
            {
                break;
            }
            const auto* description = reinterpret_cast<const std::uint8_t*>(data + cursor);
            cursor += align4(description_size);

            if (header.n_type == NT_GNU_BUILD_ID && name_size >= 3u && std::memcmp(name, "GNU", 3u) == 0)
            {
                return bytes_to_hex(description, description_size);
            }
        }
        return {};
    }

    class MappedFile
    {
      public:
        explicit MappedFile(const std::string& path)
        {
            m_descriptor = open(path.c_str(), O_RDONLY | O_CLOEXEC);
            if (m_descriptor < 0)
            {
                throw std::runtime_error("open(" + path + "): " + std::strerror(errno));
            }
            struct stat metadata{};
            if (fstat(m_descriptor, &metadata) != 0 || metadata.st_size <= 0)
            {
                const int error = errno;
                close(m_descriptor);
                m_descriptor = -1;
                throw std::runtime_error("fstat(" + path + "): " + std::strerror(error != 0 ? error : EINVAL));
            }
            m_size = static_cast<std::size_t>(metadata.st_size);
            void* mapping = mmap(nullptr, m_size, PROT_READ, MAP_PRIVATE, m_descriptor, 0);
            if (mapping == MAP_FAILED)
            {
                const int error = errno;
                close(m_descriptor);
                m_descriptor = -1;
                throw std::runtime_error("mmap(" + path + "): " + std::strerror(error));
            }
            m_data = static_cast<const std::byte*>(mapping);
        }

        MappedFile(const MappedFile&) = delete;
        MappedFile& operator=(const MappedFile&) = delete;

        ~MappedFile()
        {
            if (m_data != nullptr)
            {
                munmap(const_cast<std::byte*>(m_data), m_size);
            }
            if (m_descriptor >= 0)
            {
                close(m_descriptor);
            }
        }

        [[nodiscard]] const std::byte* data() const
        {
            return m_data;
        }
        [[nodiscard]] std::size_t size() const
        {
            return m_size;
        }

      private:
        int m_descriptor{-1};
        const std::byte* m_data{};
        std::size_t m_size{};
    };

    struct ModuleCollection
    {
        std::string executable_path;
        std::vector<ue4ss::linux::Module> modules;
    };

    int collect_module(dl_phdr_info* info, std::size_t, void* opaque)
    {
        auto& collection = *static_cast<ModuleCollection*>(opaque);
        ue4ss::linux::Module module;
        module.path = info->dlpi_name != nullptr && info->dlpi_name[0] != '\0' ? info->dlpi_name : collection.executable_path;
        module.load_bias = static_cast<std::uintptr_t>(info->dlpi_addr);

        for (Elf64_Half index = 0; index < info->dlpi_phnum; ++index)
        {
            const Elf64_Phdr& header = info->dlpi_phdr[index];
            if (header.p_type == PT_LOAD)
            {
                module.segments.push_back({
                        .address = module.load_bias + static_cast<std::uintptr_t>(header.p_vaddr),
                        .file_offset = header.p_offset,
                        .file_size = header.p_filesz,
                        .memory_size = header.p_memsz,
                        .readable = (header.p_flags & PF_R) != 0,
                        .writable = (header.p_flags & PF_W) != 0,
                        .executable = (header.p_flags & PF_X) != 0,
                });
            }
            else if (header.p_type == PT_NOTE && module.build_id.empty())
            {
                const auto* notes = reinterpret_cast<const std::byte*>(module.load_bias + static_cast<std::uintptr_t>(header.p_vaddr));
                module.build_id = find_build_id(notes, static_cast<std::size_t>(header.p_memsz));
            }
        }
        collection.modules.emplace_back(std::move(module));
        return 0;
    }
} // namespace

namespace ue4ss::linux
{
    std::string current_executable_path()
    {
        std::array<char, 4096> buffer{};
        const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size() - 1u);
        if (length < 0)
        {
            throw std::runtime_error(std::string{"readlink(/proc/self/exe): "} + std::strerror(errno));
        }
        if (static_cast<std::size_t>(length) == buffer.size() - 1u)
        {
            throw std::runtime_error("readlink(/proc/self/exe): path exceeds the supported length");
        }
        buffer[static_cast<std::size_t>(length)] = '\0';
        return {buffer.data(), static_cast<std::size_t>(length)};
    }

    ElfImage inspect_elf_file(const std::string& path)
    {
        MappedFile file{path};
        if (!range_fits(0, sizeof(Elf64_Ehdr), file.size()))
        {
            throw std::runtime_error(path + " is smaller than an ELF header");
        }

        Elf64_Ehdr header{};
        std::memcpy(&header, file.data(), sizeof(header));
        if (std::memcmp(header.e_ident, ELFMAG, SELFMAG) != 0)
        {
            throw std::runtime_error(path + " is not an ELF image");
        }
        if (header.e_ident[EI_CLASS] != ELFCLASS64 || header.e_ident[EI_DATA] != ELFDATA2LSB)
        {
            throw std::runtime_error(path + " is not a little-endian 64-bit ELF image");
        }
        if (header.e_phentsize != sizeof(Elf64_Phdr))
        {
            throw std::runtime_error(path + " uses an unsupported program-header size");
        }
        if (!range_fits(header.e_phoff, static_cast<std::size_t>(header.e_phnum) * sizeof(Elf64_Phdr), file.size()))
        {
            throw std::runtime_error(path + " has an invalid program-header table");
        }

        ElfImage image;
        image.path = path;
        image.machine = header.e_machine;
        image.elf_class = 64u;
        std::array<char, 65> sha{};
        const int sha_error = ue4ss_sha256_file(path.c_str(), sha.data());
        if (sha_error != 0)
        {
            throw std::runtime_error("sha256(" + path + "): " + std::strerror(sha_error));
        }
        image.sha256 = sha.data();

        for (Elf64_Half index = 0; index < header.e_phnum; ++index)
        {
            Elf64_Phdr program_header{};
            const std::size_t offset = static_cast<std::size_t>(header.e_phoff) + static_cast<std::size_t>(index) * sizeof(program_header);
            std::memcpy(&program_header, file.data() + offset, sizeof(program_header));
            if (program_header.p_type == PT_LOAD)
            {
                image.segments.push_back({
                        .address = static_cast<std::uintptr_t>(program_header.p_vaddr),
                        .file_offset = program_header.p_offset,
                        .file_size = program_header.p_filesz,
                        .memory_size = program_header.p_memsz,
                        .readable = (program_header.p_flags & PF_R) != 0,
                        .writable = (program_header.p_flags & PF_W) != 0,
                        .executable = (program_header.p_flags & PF_X) != 0,
                });
            }
            else if (program_header.p_type == PT_NOTE && image.build_id.empty())
            {
                if (!range_fits(program_header.p_offset, program_header.p_filesz, file.size()))
                {
                    throw std::runtime_error(path + " has an invalid PT_NOTE segment");
                }
                image.build_id = find_build_id(file.data() + program_header.p_offset, static_cast<std::size_t>(program_header.p_filesz));
            }
        }
        return image;
    }

    ElfImage inspect_current_process()
    {
        const std::string executable = current_executable_path();
        ElfImage image = inspect_elf_file(executable);
        ModuleCollection collection{.executable_path = executable};
        dl_iterate_phdr(collect_module, &collection);
        image.modules = std::move(collection.modules);
        for (const auto& module : image.modules)
        {
            if (module.path == executable)
            {
                image.build_id = module.build_id.empty() ? image.build_id : module.build_id;
                image.segments = module.segments;
                break;
            }
        }
        return image;
    }
} // namespace ue4ss::linux
