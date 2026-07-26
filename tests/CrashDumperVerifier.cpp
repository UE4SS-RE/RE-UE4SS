#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }

    const std::filesystem::path output_dir{argv[1]};
    for (const auto& entry : std::filesystem::directory_iterator{output_dir})
    {
        if (!entry.is_regular_file() || !entry.path().filename().string().starts_with("crash_"))
        {
            continue;
        }

        std::ifstream stream{entry.path()};
        const std::string contents{std::istreambuf_iterator<char>{stream}, std::istreambuf_iterator<char>{}};
        if (contents.contains("signal 6") && contents.contains("Backtrace:"))
        {
            return 0;
        }
    }
    return 1;
}
