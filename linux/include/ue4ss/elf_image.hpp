#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace ue4ss::linux
{
    struct Segment
    {
        std::uintptr_t address{};
        std::uint64_t file_offset{};
        std::uint64_t file_size{};
        std::uint64_t memory_size{};
        bool readable{};
        bool writable{};
        bool executable{};
    };

    struct Module
    {
        std::string path;
        std::string build_id;
        std::uintptr_t load_bias{};
        std::vector<Segment> segments;
    };

    struct ElfImage
    {
        std::string path;
        std::string sha256;
        std::string build_id;
        std::uint16_t machine{};
        std::uint8_t elf_class{};
        std::vector<Segment> segments;
        std::vector<Module> modules;
    };

    [[nodiscard]] std::string current_executable_path();
    [[nodiscard]] ElfImage inspect_elf_file(const std::string& path);
    [[nodiscard]] ElfImage inspect_current_process();
} // namespace ue4ss::linux
