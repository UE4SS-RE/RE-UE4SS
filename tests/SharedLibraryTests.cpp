#include <Platform/SharedLibrary.hpp>

#include <filesystem>

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        return 2;
    }

    RC::Platform::SharedLibrary library{std::filesystem::path{argv[1]}};
    if (!library.is_open())
    {
        return 3;
    }

    using AnswerFunction = int (*)();
    auto answer = reinterpret_cast<AnswerFunction>(library.resolve("shared_library_fixture_answer"));
    if (!answer || answer() != 42)
    {
        return 4;
    }

    library.close();
    if (library.is_open())
    {
        return 5;
    }

    RC::Platform::SharedLibrary missing{std::filesystem::path{argv[1]}.parent_path() / "missing-library.so"};
    if (missing.is_open() || missing.error().empty())
    {
        return 6;
    }
    return 0;
}
