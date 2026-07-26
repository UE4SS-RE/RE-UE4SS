#ifdef __linux__

#include <Platform/Linux/Diagnostics.hpp>

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iterator>
#include <string>
#include <tuple>

#include <elf.h>
#include <gnu/libc-version.h>
#include <link.h>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <SigScanner/SinglePassSigScanner.hpp>

namespace
{
    class Sha256
    {
      private:
        static constexpr std::array<uint32_t, 64> s_round_constants{
                0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5, 0xd807aa98, 0x12835b01, 0x243185be,
                0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174, 0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa,
                0x5cb0a9dc, 0x76f988da, 0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967, 0x27b70a85,
                0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85, 0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
                0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070, 0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f,
                0x682e6ff3, 0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2,
        };

        std::array<uint32_t, 8> m_state{
                0x6a09e667,
                0xbb67ae85,
                0x3c6ef372,
                0xa54ff53a,
                0x510e527f,
                0x9b05688c,
                0x1f83d9ab,
                0x5be0cd19,
        };
        std::array<uint8_t, 64> m_buffer{};
        uint64_t m_total_size{};
        size_t m_buffer_size{};

        static auto load_be32(const uint8_t* data) -> uint32_t
        {
            return (static_cast<uint32_t>(data[0]) << 24) | (static_cast<uint32_t>(data[1]) << 16) | (static_cast<uint32_t>(data[2]) << 8) |
                   static_cast<uint32_t>(data[3]);
        }

        auto transform(const uint8_t* block) -> void
        {
            std::array<uint32_t, 64> words{};
            for (size_t index = 0; index < 16; ++index)
            {
                words[index] = load_be32(block + index * 4);
            }
            for (size_t index = 16; index < words.size(); ++index)
            {
                const auto sigma0 = std::rotr(words[index - 15], 7) ^ std::rotr(words[index - 15], 18) ^ (words[index - 15] >> 3);
                const auto sigma1 = std::rotr(words[index - 2], 17) ^ std::rotr(words[index - 2], 19) ^ (words[index - 2] >> 10);
                words[index] = words[index - 16] + sigma0 + words[index - 7] + sigma1;
            }

            auto a = m_state[0];
            auto b = m_state[1];
            auto c = m_state[2];
            auto d = m_state[3];
            auto e = m_state[4];
            auto f = m_state[5];
            auto g = m_state[6];
            auto h = m_state[7];

            for (size_t index = 0; index < words.size(); ++index)
            {
                const auto sum1 = std::rotr(e, 6) ^ std::rotr(e, 11) ^ std::rotr(e, 25);
                const auto choose = (e & f) ^ (~e & g);
                const auto temp1 = h + sum1 + choose + s_round_constants[index] + words[index];
                const auto sum0 = std::rotr(a, 2) ^ std::rotr(a, 13) ^ std::rotr(a, 22);
                const auto majority = (a & b) ^ (a & c) ^ (b & c);
                const auto temp2 = sum0 + majority;

                h = g;
                g = f;
                f = e;
                e = d + temp1;
                d = c;
                c = b;
                b = a;
                a = temp1 + temp2;
            }

            m_state[0] += a;
            m_state[1] += b;
            m_state[2] += c;
            m_state[3] += d;
            m_state[4] += e;
            m_state[5] += f;
            m_state[6] += g;
            m_state[7] += h;
        }

      public:
        auto update(const uint8_t* data, size_t size) -> void
        {
            m_total_size += size;
            while (size > 0)
            {
                const auto copy_size = std::min(size, m_buffer.size() - m_buffer_size);
                std::memcpy(m_buffer.data() + m_buffer_size, data, copy_size);
                m_buffer_size += copy_size;
                data += copy_size;
                size -= copy_size;

                if (m_buffer_size == m_buffer.size())
                {
                    transform(m_buffer.data());
                    m_buffer_size = 0;
                }
            }
        }

        auto finalize() -> std::string
        {
            const auto bit_length = m_total_size * 8;
            m_buffer[m_buffer_size++] = 0x80;
            if (m_buffer_size > 56)
            {
                std::fill(m_buffer.begin() + static_cast<ptrdiff_t>(m_buffer_size), m_buffer.end(), 0);
                transform(m_buffer.data());
                m_buffer_size = 0;
            }
            std::fill(m_buffer.begin() + static_cast<ptrdiff_t>(m_buffer_size), m_buffer.begin() + 56, 0);
            for (size_t index = 0; index < 8; ++index)
            {
                m_buffer[56 + index] = static_cast<uint8_t>(bit_length >> (56 - index * 8));
            }
            transform(m_buffer.data());

            std::string digest;
            digest.reserve(64);
            for (const auto word : m_state)
            {
                std::array<char, 9> chunk{};
                std::snprintf(chunk.data(), chunk.size(), "%08x", word);
                digest.append(chunk.data(), 8);
            }
            return digest;
        }
    };

    auto sha256_file(const std::filesystem::path& path) -> std::string
    {
        std::ifstream file{path, std::ios::binary};
        if (!file)
        {
            return "unavailable";
        }

        Sha256 sha256;
        std::array<uint8_t, 65536> buffer{};
        while (file)
        {
            file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
            const auto bytes_read = file.gcount();
            if (bytes_read > 0)
            {
                sha256.update(buffer.data(), static_cast<size_t>(bytes_read));
            }
        }
        return sha256.finalize();
    }

    auto find_libstdcxx(struct dl_phdr_info* info, size_t, void* user_data) -> int
    {
        if (info->dlpi_name && std::strstr(info->dlpi_name, "libstdc++.so"))
        {
            *static_cast<std::string*>(user_data) = info->dlpi_name;
            return 1;
        }
        return 0;
    }

    auto glibcxx_ceiling() -> std::string
    {
        std::string library_path;
        dl_iterate_phdr(&find_libstdcxx, &library_path);
        std::ifstream file{library_path, std::ios::binary};
        if (!file)
        {
            return "unavailable";
        }

        const std::string contents{std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
        constexpr std::string_view prefix{"GLIBCXX_"};
        std::array<unsigned long, 3> best{};
        std::string best_string{"unavailable"};
        for (size_t position = 0; (position = contents.find(prefix, position)) != std::string::npos; ++position)
        {
            const auto version_start = position + prefix.size();
            std::array<unsigned long, 3> current{};
            size_t cursor = version_start;
            size_t part_count{};
            while (part_count < current.size() && cursor < contents.size() && std::isdigit(static_cast<unsigned char>(contents[cursor])))
            {
                unsigned long value{};
                while (cursor < contents.size() && std::isdigit(static_cast<unsigned char>(contents[cursor])))
                {
                    value = value * 10 + static_cast<unsigned long>(contents[cursor] - '0');
                    ++cursor;
                }
                current[part_count++] = value;
                if (cursor >= contents.size() || contents[cursor] != '.')
                {
                    break;
                }
                ++cursor;
            }
            if (part_count >= 2 && current > best)
            {
                best = current;
                best_string = contents.substr(position, cursor - position);
            }
        }
        return best_string;
    }
} // namespace

namespace RC::LinuxDiagnostics
{
    auto is_enabled() -> bool
    {
        const auto* diagnose = std::getenv("UE4SS_DIAGNOSE");
        return diagnose && std::strcmp(diagnose, "1") == 0;
    }

    auto output_environment(const std::filesystem::path& executable) -> void
    {
        if (!is_enabled())
        {
            return;
        }

        Output::send(STR("DIAG: executable={}\n"), ensure_str(executable));
        Output::send(STR("DIAG: executable_sha256={}\n"), ensure_str(sha256_file(executable)));
        Output::send(STR("DIAG: glibc_version={}\n"), ensure_str(gnu_get_libc_version()));
        Output::send(STR("DIAG: glibcxx_ceiling={}\n"), ensure_str(glibcxx_ceiling()));

        const auto& main_module = SigScannerStaticData::m_modules_info[ScanTarget::MainExe];
        for (const auto& [segment_start, segment_size, flags] : main_module.readable_segments)
        {
            const auto start = reinterpret_cast<uintptr_t>(segment_start);
            const auto end = start + segment_size;
            std::array<char, 4> protection{
                    (flags & PF_R) != 0 ? 'R' : '-',
                    (flags & PF_W) != 0 ? 'W' : '-',
                    (flags & PF_X) != 0 ? 'X' : '-',
                    '\0',
            };
            Output::send(STR("DIAG: module=MainExe range=0x{:x}-0x{:x} flags={}\n"), start, end, ensure_str(protection.data()));
        }
    }

    auto output_inactive_reason(std::string_view reason) -> void
    {
        if (is_enabled())
        {
            Output::send<LogLevel::Error>(STR("DIAG: inactive_reason={}\n"), ensure_str(reason));
        }
    }
} // namespace RC::LinuxDiagnostics

#endif
