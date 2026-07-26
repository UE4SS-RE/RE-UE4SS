#include <CrashDumper.hpp>

#include <csignal>
#include <cstdlib>
#include <filesystem>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }

    const std::filesystem::path output_dir{argv[1]};
    std::filesystem::remove_all(output_dir);
    std::filesystem::create_directories(output_dir);
    setenv("UE4SS_CRASH_LOG_DIR", output_dir.c_str(), 1);

    RC::CrashDumper crash_dumper{};
    crash_dumper.enable();
    raise(SIGABRT);
    return 3;
}
