#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include <UVTD/Config.hpp>
#include <UVTD/ConfigUtil.hpp>

namespace
{
    class TemporaryConfigDirectory
    {
    public:
        explicit TemporaryConfigDirectory(std::string_view test_name)
        {
            static size_t sequence{};
            const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
            m_path = std::filesystem::temp_directory_path() /
                     ("uvtd_platform_layout_" + std::string{test_name} + "_" + std::to_string(timestamp) + "_" +
                      std::to_string(sequence++));
            std::filesystem::create_directories(m_path);
        }

        ~TemporaryConfigDirectory()
        {
            std::error_code error;
            std::filesystem::remove_all(m_path, error);
        }

        TemporaryConfigDirectory(const TemporaryConfigDirectory&) = delete;
        TemporaryConfigDirectory& operator=(const TemporaryConfigDirectory&) = delete;

        const std::filesystem::path& path() const
        {
            return m_path;
        }

        void write(std::string_view filename, std::string_view contents) const
        {
            std::ofstream file{m_path / filename};
            if (!file)
            {
                throw std::runtime_error{"Failed to create temporary configuration file"};
            }
            file << contents;
            if (!file)
            {
                throw std::runtime_error{"Failed to write temporary configuration file"};
            }
        }

    private:
        std::filesystem::path m_path;
    };

    std::string make_layout_entry(std::string_view platform = "Linux",
                                  std::string_view ifdef_macro = "PLATFORM_LINUX",
                                  std::string_view version = "5_01",
                                  std::string_view class_name = "FProperty",
                                  std::string_view member_name = "ArrayDim",
                                  bool include_total_size = true)
    {
        std::string json = "{\"platform\":\"" + std::string{platform} + "\",\"ifdef_macro\":\"" +
                           std::string{ifdef_macro} + "\",\"version\":\"" + std::string{version} +
                           "\",\"classes\":{\"" + std::string{class_name} + "\":{\"" +
                           std::string{member_name} + "\":52";
        if (include_total_size)
        {
            json += ",\"UEP_TotalSize\":120";
        }
        json += "}}}";
        return json;
    }

    std::string make_layout_file(std::string_view platform = "Linux",
                                 std::string_view ifdef_macro = "PLATFORM_LINUX",
                                 std::string_view version = "5_01",
                                 std::string_view class_name = "FProperty",
                                 std::string_view member_name = "ArrayDim",
                                 bool include_total_size = true)
    {
        return "{\"layouts\":[" +
               make_layout_entry(platform, ifdef_macro, version, class_name, member_name, include_total_size) + "]}";
    }

    std::string make_object_items(std::string_view marker)
    {
        return "[{\"name\":\"" + std::string{marker} +
               "\",\"valid_for_vtable\":1,\"valid_for_member_vars\":1}]";
    }

    class TestRunner
    {
    public:
        void expect(bool condition, std::string_view description)
        {
            if (!condition)
            {
                std::cerr << "FAIL: " << description << '\n';
                ++m_failures;
            }
        }

        int result() const
        {
            if (m_failures == 0)
            {
                std::cout << "All platform member layout configuration tests passed\n";
            }
            return m_failures == 0 ? 0 : 1;
        }

    private:
        int m_failures{};
    };
}

int main()
{
    using namespace RC::UVTD;

    try
    {
        TestRunner tests;
        auto& config = UVTDConfig::Get();

        {
            TemporaryConfigDirectory directory{"valid"};
            directory.write("platform_member_layouts.json", make_layout_file());

            tests.expect(config.Initialize(directory.path()), "valid platform layout should load");
            const auto matches = ConfigUtil::GetPlatformMemberLayoutsForClass(STR("5_01"), STR("FProperty"));
            tests.expect(matches.size() == 1, "lookup should return the matching class layout");
            if (matches.size() == 1)
            {
                tests.expect(matches[0]->platform == STR("Linux"), "lookup should return the Linux layout");
                tests.expect(matches[0]->classes.at(STR("FProperty")).at(STR("ArrayDim")) == 52,
                             "loaded layout should preserve member offsets");
            }
        }

        {
            TemporaryConfigDirectory directory{"missing_layouts"};
            directory.write("platform_member_layouts.json", "{}");
            tests.expect(!config.Initialize(directory.path()), "missing top-level layouts should fail");
        }

        {
            TemporaryConfigDirectory directory{"empty_layouts"};
            directory.write("platform_member_layouts.json", "{\"layouts\":[]}");
            tests.expect(!config.Initialize(directory.path()), "empty top-level layouts should fail");
        }

        {
            TemporaryConfigDirectory directory{"duplicate"};
            const auto entry = make_layout_entry();
            directory.write("platform_member_layouts.json", "{\"layouts\":[" + entry + "," + entry + "]}");
            tests.expect(!config.Initialize(directory.path()), "duplicate platform and version should fail");
        }

        for (const auto& [name, json] : {
                 std::pair{"unsafe_platform_parent", make_layout_file("../Linux")},
                 std::pair{"unsafe_platform_whitespace", make_layout_file("Linux x")},
                 std::pair{"unsafe_version_parent", make_layout_file("Linux", "PLATFORM_LINUX", "../5_01")},
                 std::pair{"unsafe_version_separator", make_layout_file("Linux", "PLATFORM_LINUX", "5/01")},
             })
        {
            TemporaryConfigDirectory directory{name};
            directory.write("platform_member_layouts.json", json);
            tests.expect(!config.Initialize(directory.path()), "unsafe platform/version path token should fail");
        }

        for (const auto& [name, json] : {
                 std::pair{"invalid_macro", make_layout_file("Linux", "PLATFORM-LINUX")},
                 std::pair{"invalid_macro_leading_digit", make_layout_file("Linux", "1PLATFORM_LINUX")},
                 std::pair{"invalid_class", make_layout_file("Linux", "PLATFORM_LINUX", "5_01", "F::Property")},
                 std::pair{"invalid_class_leading_digit",
                           make_layout_file("Linux", "PLATFORM_LINUX", "5_01", "1Property")},
                 std::pair{"invalid_member",
                           make_layout_file("Linux", "PLATFORM_LINUX", "5_01", "FProperty", "Offset-Internal")},
                 std::pair{"invalid_member_leading_digit",
                           make_layout_file("Linux", "PLATFORM_LINUX", "5_01", "FProperty", "1Offset")},
             })
        {
            TemporaryConfigDirectory directory{name};
            directory.write("platform_member_layouts.json", json);
            tests.expect(!config.Initialize(directory.path()), "invalid macro/class/member identifier should fail");
        }

        {
            TemporaryConfigDirectory directory{"missing_total_size"};
            directory.write("platform_member_layouts.json",
                            make_layout_file("Linux", "PLATFORM_LINUX", "5_01", "FProperty", "ArrayDim", false));
            tests.expect(!config.Initialize(directory.path()), "class without UEP_TotalSize should fail");
        }

        {
            TemporaryConfigDirectory directory{"missing_file"};
            tests.expect(!config.Initialize(directory.path()), "missing required platform layout file should fail");
        }

        {
            TemporaryConfigDirectory valid_directory{"transaction_valid"};
            valid_directory.write("object_items.json", make_object_items("OriginalMarker"));
            valid_directory.write("platform_member_layouts.json", make_layout_file());
            tests.expect(config.Initialize(valid_directory.path()), "transaction baseline should load");

            TemporaryConfigDirectory invalid_directory{"transaction_invalid"};
            invalid_directory.write("object_items.json", make_object_items("ReplacementMarker"));
            invalid_directory.write("platform_member_layouts.json", "{\"layouts\":[]}");
            tests.expect(!config.Initialize(invalid_directory.path()), "invalid reload should fail");
            tests.expect(config.object_items.size() == 1 && config.object_items[0].name == STR("OriginalMarker"),
                         "failed reload should preserve previous object items");
            tests.expect(config.platform_member_layouts.size() == 1 &&
                             config.platform_member_layouts[0].platform == STR("Linux") &&
                             config.platform_member_layouts[0].version == STR("5_01"),
                         "failed reload should preserve previous platform layouts");
        }

        return tests.result();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Unexpected test exception: " << exception.what() << '\n';
        return 1;
    }
}
