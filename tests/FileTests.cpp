#include <cstdio>
#include <filesystem>
#include <string>

#include <File/File.hpp>
#include <Helpers/String.hpp>

using namespace RC;

#define CHECK(cond)                                                                                                                                            \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if (!(cond))                                                                                                                                           \
        {                                                                                                                                                      \
            fprintf(stderr, "FAILED: %s at %s:%d\n", #cond, __FILE__, __LINE__);                                                                               \
            return 1;                                                                                                                                          \
        }                                                                                                                                                      \
    } while (0)

static auto test_write_and_read_roundtrip(const std::filesystem::path& dir) -> int
{
    auto path = dir / "ue4ss_file_test.txt";
    {
        auto handle = File::open(path, File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        handle.write_string_to_file(STR("hello linux åäö\n"));
        handle.close();
    }
    {
        auto handle = File::open(path, File::OpenFor::Reading);
        auto contents = handle.read_all();
        CHECK(contents == StringType{STR("hello linux åäö\n")});
        handle.close();
    }
    return 0;
}

static auto test_append(const std::filesystem::path& dir) -> int
{
    auto path = dir / "ue4ss_append_test.txt";
    std::filesystem::remove(path);
    {
        auto h = File::open(path, File::OpenFor::Appending, File::OverwriteExistingFile::No, File::CreateIfNonExistent::Yes);
        h.write_string_to_file(STR("a"));
        h.close();
    }
    {
        auto h = File::open(path, File::OpenFor::Appending, File::OverwriteExistingFile::No, File::CreateIfNonExistent::Yes);
        h.write_string_to_file(STR("b"));
        h.close();
    }
    auto h = File::open(path, File::OpenFor::Reading);
    CHECK(h.read_all() == StringType{STR("ab")});
    h.close();
    return 0;
}

static auto test_memory_map(const std::filesystem::path& dir) -> int
{
    auto path = dir / "ue4ss_mmap_test.bin";
    {
        auto h = File::open(path, File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        h.write_string_to_file(STR("mmap-data"));
        h.close();
    }
    auto h = File::open(path, File::OpenFor::Reading);
    auto span = h.memory_map();
    CHECK(span.size() >= 9);
    CHECK(span[0] == 'm' && span[8] == 'a');
    h.close();
    return 0;
}

static auto test_open_missing_throws(const std::filesystem::path& dir) -> int
{
    bool threw = false;
    try
    {
        auto h = File::open(dir / "does_not_exist_ue4ss.txt", File::OpenFor::Reading);
    }
    catch (const std::exception&)
    {
        threw = true;
    }
    CHECK(threw);
    return 0;
}

static auto test_delete(const std::filesystem::path& dir) -> int
{
    auto path = dir / "ue4ss_delete_test.txt";
    {
        auto h = File::open(path, File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        h.write_string_to_file(STR("x"));
        h.close();
    }
    File::delete_file(path);
    CHECK(!std::filesystem::exists(path));
    return 0;
}

static auto test_create_nested_directories(const std::filesystem::path& dir) -> int
{
    auto nested = dir / "ue4ss_nested_a" / "b" / "c" / "file.txt";
    std::filesystem::remove_all(dir / "ue4ss_nested_a");
    {
        auto h = File::open(nested, File::OpenFor::Writing, File::OverwriteExistingFile::Yes, File::CreateIfNonExistent::Yes);
        h.write_string_to_file(STR("nested"));
        h.close();
    }
    CHECK(std::filesystem::exists(nested));
    std::filesystem::remove_all(dir / "ue4ss_nested_a");
    return 0;
}

int main()
{
    auto dir = std::filesystem::temp_directory_path();
    int rc = 0;
    rc |= test_write_and_read_roundtrip(dir);
    rc |= test_append(dir);
    rc |= test_memory_map(dir);
    rc |= test_open_missing_throws(dir);
    rc |= test_delete(dir);
    rc |= test_create_nested_directories(dir);
    if (rc == 0)
    {
        printf("FileTests: all passed\n");
    }
    return rc;
}
