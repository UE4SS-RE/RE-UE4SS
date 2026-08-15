#include "ue4ss/elf_image.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace
{
    [[nodiscard]] std::string json_escape(std::string_view input)
    {
        std::string output;
        for (char character : input)
        {
            if (character == '"' || character == '\\')
            {
                output.push_back('\\');
            }
            output.push_back(character);
        }
        return output;
    }
} // namespace

int main(int argc, char** argv)
{
    bool json = false;
    std::string path;
    for (int index = 1; index < argc; ++index)
    {
        if (std::string_view{argv[index]} == "--json")
        {
            json = true;
        }
        else if (path.empty())
        {
            path = argv[index];
        }
        else
        {
            std::cerr << "usage: ue4ss_probe [--json] <elf-image>\n";
            return 64;
        }
    }
    if (path.empty())
    {
        std::cerr << "usage: ue4ss_probe [--json] <elf-image>\n";
        return 64;
    }

    try
    {
        const auto image = ue4ss::linux::inspect_elf_file(path);
        if (json)
        {
            std::cout << "{\"path\":\"" << json_escape(image.path) << "\",\"sha256\":\"" << image.sha256 << "\",\"build_id\":\"" << image.build_id
                      << "\",\"elf_class\":" << static_cast<unsigned int>(image.elf_class) << ",\"machine\":" << image.machine
                      << ",\"segments\":" << image.segments.size() << "}\n";
        }
        else
        {
            std::cout << "Path: " << image.path << '\n'
                      << "SHA-256: " << image.sha256 << '\n'
                      << "Build ID: " << image.build_id << '\n'
                      << "ELF: " << static_cast<unsigned int>(image.elf_class) << "-bit, machine " << image.machine << '\n'
                      << "Load segments: " << image.segments.size() << '\n';
        }
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "ue4ss_probe: " << error.what() << '\n';
        return 65;
    }
}
