#include <filesystem>

#include <SDKGenerator/Generator.hpp>

int main()
{
    const std::filesystem::path expected{"/tmp/ue4ss/CXXHeaderDump"};
    const auto resolved = RC::UEGenerator::resolve_output_directory(expected);
    return resolved == expected && resolved.is_absolute() ? 0 : 1;
}
