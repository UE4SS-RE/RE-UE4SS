#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <UVTD/Config.hpp>
#include <UVTD/Helpers.hpp>
#include <UVTD/PDBNameInfo.hpp>
#include <UVTD/PlatformMemberLayoutGenerator.hpp>
#include <UVTD/TypeContainer.hpp>
#include <UVTD/UnrealVirtualGenerator.hpp>

namespace
{
    class ArtifactDirectory
    {
    public:
        explicit ArtifactDirectory(const std::filesystem::path* preservation_root)
            : m_preserve{preservation_root != nullptr}
        {
            if (m_preserve)
            {
                m_path = std::filesystem::absolute(*preservation_root);
                if (std::filesystem::exists(m_path))
                {
                    throw std::runtime_error{
                        "Artifact output root already exists; refusing to modify an unowned destination"};
                }
            }
            else
            {
                static size_t sequence{};
                const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
                m_path = std::filesystem::temp_directory_path() /
                         ("uvtd_platform_layout_generator_" + std::to_string(timestamp) + "_" +
                          std::to_string(sequence++));
            }

            std::filesystem::create_directories(m_path);
        }

        ~ArtifactDirectory()
        {
            if (!m_preserve)
            {
                std::error_code error;
                std::filesystem::remove_all(m_path, error);
            }
        }

        ArtifactDirectory(const ArtifactDirectory&) = delete;
        ArtifactDirectory& operator=(const ArtifactDirectory&) = delete;

        const std::filesystem::path& path() const
        {
            return m_path;
        }

        bool preserves_artifacts() const
        {
            return m_preserve;
        }

    private:
        std::filesystem::path m_path;
        bool m_preserve{};
    };

    class CurrentDirectoryGuard
    {
    public:
        explicit CurrentDirectoryGuard(const std::filesystem::path& path)
            : m_original_path{std::filesystem::current_path()}
        {
            std::filesystem::current_path(path);
        }

        ~CurrentDirectoryGuard()
        {
            std::error_code error;
            std::filesystem::current_path(m_original_path, error);
        }

        CurrentDirectoryGuard(const CurrentDirectoryGuard&) = delete;
        CurrentDirectoryGuard& operator=(const CurrentDirectoryGuard&) = delete;

    private:
        std::filesystem::path m_original_path;
    };

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
                std::cout << "All platform member layout generator tests passed\n";
            }
            return m_failures == 0 ? 0 : 1;
        }

    private:
        int m_failures{};
    };

    std::string read_file(const std::filesystem::path& path)
    {
        std::ifstream file{path, std::ios::binary};
        if (!file)
        {
            throw std::runtime_error{"Failed to open generated file"};
        }
        return {std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
    }

    void write_file(const std::filesystem::path& path, std::string_view contents)
    {
        std::filesystem::create_directories(path.parent_path());
        std::ofstream file{path, std::ios::binary};
        if (!file)
        {
            throw std::runtime_error{"Failed to create test file"};
        }
        file << contents;
        if (!file)
        {
            throw std::runtime_error{"Failed to write test file"};
        }
    }

    template <typename Callable>
    std::string exception_message(Callable&& callable)
    {
        try
        {
            std::forward<Callable>(callable)();
        }
        catch (const std::exception& exception)
        {
            return exception.what();
        }
        return {};
    }

    using GeneratedFiles = std::vector<std::pair<std::string, std::string>>;

    GeneratedFiles snapshot_files(const std::filesystem::path& root)
    {
        GeneratedFiles files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator{root})
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            files.emplace_back(std::filesystem::relative(entry.path(), root).generic_string(), read_file(entry.path()));
        }
        std::ranges::sort(files);
        return files;
    }

    using MemberOffset = std::pair<const wchar_t*, uint32_t>;

    std::unordered_map<RC::File::StringType, uint32_t> make_members(
        std::vector<MemberOffset> members,
        bool reverse_insertion)
    {
        if (reverse_insertion)
        {
            std::ranges::reverse(members);
        }

        std::unordered_map<RC::File::StringType, uint32_t> result;
        for (const auto& [name, offset] : members)
        {
            result.emplace(name, offset);
        }
        return result;
    }

    RC::UVTD::PlatformMemberLayout make_linux_layout(bool reverse_insertion)
    {
        using namespace RC::UVTD;

        const std::vector<MemberOffset> fproperty_members{
            {STR("ArrayDim"), 0x34},
            {STR("DestructorLinkNext"), 0x68},
            {STR("ElementSize"), 0x38},
            {STR("NextRef"), 0x60},
            {STR("Offset_Internal"), 0x4C},
            {STR("PostConstructLinkNext"), 0x70},
            {STR("PropertyFlags"), 0x40},
            {STR("PropertyLinkNext"), 0x58},
            {STR("RepIndex"), 0x48},
            {STR("RepNotifyFunc"), 0x50},
            {STR("UEP_TotalSize"), 0x78},
        };
        const std::vector<MemberOffset> uenum_members{
            {STR("CppForm"), 0x40},
            {STR("CppType"), 0x30},
            {STR("EnumDisplayNameFn"), 0x60},
            {STR("EnumFlags_Internal"), 0x58},
            {STR("EnumPackage"), 0x68},
            {STR("Names"), 0x48},
            {STR("UEP_TotalSize"), 0x70},
        };

        PlatformMemberLayout layout{
            .platform = STR("Linux"),
            .ifdef_macro = STR("PLATFORM_LINUX"),
            .version = STR("5_01"),
        };

        if (reverse_insertion)
        {
            layout.classes.emplace(STR("UEnum"), make_members(uenum_members, true));
            layout.classes.emplace(STR("FProperty"), make_members(fproperty_members, true));
        }
        else
        {
            layout.classes.emplace(STR("FProperty"), make_members(fproperty_members, false));
            layout.classes.emplace(STR("UEnum"), make_members(uenum_members, false));
        }
        return layout;
    }

    RC::UVTD::PlatformMemberLayout make_test_layout(bool reverse_insertion)
    {
        using namespace RC::UVTD;

        PlatformMemberLayout layout{
            .platform = STR("TestPlatform"),
            .ifdef_macro = STR("PLATFORM_TEST"),
            .version = STR("4_27"),
        };
        const std::vector<MemberOffset> alpha_members{
            {STR("First"), 0x10},
            {STR("UEP_TotalSize"), 0x20},
        };
        const std::vector<MemberOffset> zeta_members{
            {STR("Last"), 0x18},
            {STR("UEP_TotalSize"), 0x28},
        };

        if (reverse_insertion)
        {
            layout.classes.emplace(STR("Zeta"), make_members(zeta_members, true));
            layout.classes.emplace(STR("Alpha"), make_members(alpha_members, true));
        }
        else
        {
            layout.classes.emplace(STR("Alpha"), make_members(alpha_members, false));
            layout.classes.emplace(STR("Zeta"), make_members(zeta_members, false));
        }
        return layout;
    }
}

int main(int argc, char** argv)
{
    using namespace RC::UVTD;

    if (argc > 2)
    {
        std::cerr << "Usage: PlatformMemberLayoutGeneratorTests [artifact-output-root]\n";
        return 2;
    }

    try
    {
        std::filesystem::path preservation_root;
        const std::filesystem::path* preservation_root_ptr{};
        if (argc == 2)
        {
            preservation_root = argv[1];
            preservation_root_ptr = &preservation_root;
        }

        ArtifactDirectory artifacts{preservation_root_ptr};
        CurrentDirectoryGuard current_directory{artifacts.path()};
        TestRunner tests;

        const auto platform_output_root = std::filesystem::absolute(platform_member_layouts_output_path);
        const auto stale_file = platform_output_root / "StalePlatform" / "stale.cpp";
        write_file(stale_file, "stale");

        auto linux_layout = make_linux_layout(true);
        PlatformMemberLayoutGenerator generator{{linux_layout}, platform_output_root};
        generator.output_cleanup();
        tests.expect(!std::filesystem::exists(platform_output_root),
                     "cleanup should remove the entire generator-owned platform root");

        generator.generate_files();

        const auto fproperty_file =
            platform_output_root / "Linux" / "5_01_MemberVariableLayout_DefaultSetter_FProperty.cpp";
        const auto uenum_file =
            platform_output_root / "Linux" / "5_01_MemberVariableLayout_DefaultSetter_UEnum.cpp";
        tests.expect(std::filesystem::exists(fproperty_file), "Linux FProperty setter should be generated at the expected path");
        tests.expect(std::filesystem::exists(uenum_file), "Linux UEnum setter should be generated at the expected path");
        tests.expect(!std::filesystem::exists(stale_file), "cleanup should remove stale platform directories");

        const auto fproperty_contents = read_file(fproperty_file);
        const std::string array_dim_statement =
            "if (auto it = FProperty::MemberOffsets.find(STR(\"ArrayDim\")); it == FProperty::MemberOffsets.end())\n"
            "{\n"
            "    FProperty::MemberOffsets.emplace(STR(\"ArrayDim\"), 0x34);\n"
            "}";
        const std::string total_size_statement =
            "if (auto it = FProperty::MemberOffsets.find(STR(\"UEP_TotalSize\")); it == FProperty::MemberOffsets.end())\n"
            "{\n"
            "    FProperty::MemberOffsets.emplace(STR(\"UEP_TotalSize\"), 0x78);\n"
            "}";
        tests.expect(fproperty_contents.contains(array_dim_statement), "ArrayDim should use the exact guarded 0x34 statement");
        tests.expect(fproperty_contents.contains(total_size_statement), "UEP_TotalSize should use the exact guarded 0x78 statement");

        const auto array_dim_position = fproperty_contents.find("STR(\"ArrayDim\")");
        const auto element_size_position = fproperty_contents.find("STR(\"ElementSize\")");
        const auto total_size_position = fproperty_contents.find("STR(\"UEP_TotalSize\")");
        tests.expect(array_dim_position < element_size_position && element_size_position < total_size_position,
                     "members should be emitted in lexical order despite reverse insertion");

        const auto deterministic_a_root = artifacts.path() / "deterministic-a";
        const auto deterministic_b_root = artifacts.path() / "deterministic-b";
        PlatformMemberLayoutGenerator deterministic_a{
            {make_linux_layout(true), make_test_layout(true)}, deterministic_a_root};
        PlatformMemberLayoutGenerator deterministic_b{
            {make_test_layout(false), make_linux_layout(false)}, deterministic_b_root};
        deterministic_a.generate_files();
        deterministic_b.generate_files();
        tests.expect(snapshot_files(deterministic_a_root) == snapshot_files(deterministic_b_root),
                     "platform, version, class, and member ordering should produce deterministic output");
        deterministic_a.output_cleanup();
        deterministic_b.output_cleanup();

        const auto exact_collision_root = artifacts.path() / "exact-collision-published";
        const auto exact_collision_marker = exact_collision_root / "marker.txt";
        write_file(exact_collision_marker, "published-tree");
        const auto exact_collision_before = snapshot_files(exact_collision_root);
        PlatformMemberLayoutGenerator exact_collision_generator{
            {make_linux_layout(false), make_linux_layout(true)}, exact_collision_root};
        const auto exact_collision_message = exception_message([&] { exact_collision_generator.generate_files(); });
        tests.expect(exact_collision_message.contains("collision"),
                     "exact duplicate relative output paths should throw a clear collision error");
        tests.expect(snapshot_files(exact_collision_root) == exact_collision_before,
                     "exact collision failure should leave the published tree intact");
        exact_collision_generator.output_cleanup();

        const auto case_collision_root = artifacts.path() / "case-collision-published";
        const auto case_collision_marker = case_collision_root / "marker.txt";
        write_file(case_collision_marker, "published-tree");
        const auto case_collision_before = snapshot_files(case_collision_root);
        auto lowercase_linux_layout = make_linux_layout(false);
        lowercase_linux_layout.platform = STR("linux");
        lowercase_linux_layout.ifdef_macro = STR("PLATFORM_LOWERCASE_LINUX");
        PlatformMemberLayoutGenerator case_collision_generator{
            {make_linux_layout(false), std::move(lowercase_linux_layout)}, case_collision_root};
        const auto case_collision_message = exception_message([&] { case_collision_generator.generate_files(); });
        tests.expect(case_collision_message.contains("collision"),
                     "ASCII case-folded output paths should throw a clear collision error");
        tests.expect(snapshot_files(case_collision_root) == case_collision_before,
                     "case-folded collision failure should leave the published tree intact");
        case_collision_generator.output_cleanup();

        const auto replacement_root = artifacts.path() / "transactional-replacement";
        const auto replacement_marker = replacement_root / "stale-platform" / "marker.txt";
        write_file(replacement_marker, "old-published-tree");
        PlatformMemberLayoutGenerator replacement_generator{{make_linux_layout(false)}, replacement_root};
        replacement_generator.generate_files();
        tests.expect(!std::filesystem::exists(replacement_marker),
                     "successful generation should transactionally replace the previous published tree");
        tests.expect(snapshot_files(replacement_root).size() == 2,
                     "successful transactional replacement should publish only the configured files");
        replacement_generator.output_cleanup();

        auto& config = UVTDConfig::Get();
        config.object_items = {
            ObjectItem{STR("FProperty"), ValidForVTable::No, ValidForMemberVars::Yes},
        };
        config.virtual_generator_includes.clear();
        config.platform_member_layouts = {linux_layout};
        config.pdbs_to_dump = {std::filesystem::path{STR("5_01_CasePreserving.pdb")}};
        config.suffix_definitions.clear();
        config.suffix_definitions.emplace(
            STR("CasePreserving"),
            SuffixDefinition{
                STR("CasePreserving"),
                STR("WITH_CASE_PRESERVING_NAME"),
                STR("Case-preserving name layout"),
                true,
            });

        TypeContainer type_container;
        auto& fproperty_class = type_container.get_or_create_class_entry(
            STR("FProperty"),
            STR("FProperty"),
            SymbolNameInfo{ValidForVTable::No, ValidForMemberVars::Yes});
        MemberVariable array_dim;
        array_dim.type = STR("int32");
        array_dim.name = STR("ArrayDim");
        array_dim.offset = 0x34;
        array_dim.size = 4;
        fproperty_class.variables.emplace_back(std::move(array_dim));

        const auto pdb_info = PDBNameInfo::parse(STR("5_01"));
        tests.expect(pdb_info.has_value(), "5_01 should parse as a real PDBNameInfo");
        if (!pdb_info.has_value())
        {
            return tests.result();
        }

        UnrealVirtualGenerator virtual_generator{*pdb_info, std::move(type_container)};
        virtual_generator.generate_files();

        const auto unreal_virtual_file = std::filesystem::absolute(virtual_gen_source_output_path) / "UnrealVirtual501.cpp";
        const auto unreal_virtual_header = std::filesystem::absolute(virtual_gen_header_output_path) / "UnrealVirtual501.hpp";
        tests.expect(std::filesystem::exists(unreal_virtual_file), "UnrealVirtual501.cpp should be generated");
        tests.expect(std::filesystem::exists(unreal_virtual_header), "UnrealVirtual501.hpp should be generated");
        const auto unreal_virtual_contents = read_file(unreal_virtual_file);

        const std::string default_selection =
            "#if PLATFORM_LINUX\n"
            "#include <FunctionBodies/Platform/Linux/5_01_MemberVariableLayout_DefaultSetter_FProperty.cpp>\n"
            "#else\n"
            "#include <FunctionBodies/5_01_MemberVariableLayout_DefaultSetter_FProperty.cpp>\n"
            "#endif";
        tests.expect(unreal_virtual_contents.contains(default_selection),
                     "default include should select Linux layout with the existing generic fallback");

        const std::string suffix_selection =
            "#ifdef WITH_CASE_PRESERVING_NAME\n"
            "#if PLATFORM_LINUX\n"
            "#include <FunctionBodies/Platform/Linux/5_01_MemberVariableLayout_DefaultSetter_FProperty.cpp>\n"
            "#else\n"
            "#include <FunctionBodies/5_01_CasePreserving_MemberVariableLayout_DefaultSetter_FProperty.cpp>\n"
            "#endif\n"
            "#else // !WITH_CASE_PRESERVING_NAME";
        tests.expect(unreal_virtual_contents.contains(suffix_selection),
                     "suffix include should preserve its suffix-specific non-Linux fallback");

        const auto generate_virtual_contents = [&]() {
            TypeContainer container;
            auto& class_entry = container.get_or_create_class_entry(
                STR("FProperty"),
                STR("FProperty"),
                SymbolNameInfo{ValidForVTable::No, ValidForMemberVars::Yes});
            MemberVariable member;
            member.type = STR("int32");
            member.name = STR("ArrayDim");
            member.offset = 0x34;
            member.size = 4;
            class_entry.variables.emplace_back(std::move(member));

            UnrealVirtualGenerator generator_for_selection{*pdb_info, std::move(container)};
            generator_for_selection.generate_files();
            return read_file(unreal_virtual_file);
        };

        auto steam_deck_layout = linux_layout;
        steam_deck_layout.platform = STR("SteamDeck");
        steam_deck_layout.ifdef_macro = STR("PLATFORM_STEAM_DECK");
        config.suffix_definitions.clear();
        config.pdbs_to_dump.clear();
        config.platform_member_layouts = {steam_deck_layout, linux_layout};
        const auto multi_platform_contents = generate_virtual_contents();
        const auto multi_platform_header_contents = read_file(unreal_virtual_header);
        const std::string multi_platform_selection =
            "#if PLATFORM_LINUX\n"
            "#include <FunctionBodies/Platform/Linux/5_01_MemberVariableLayout_DefaultSetter_FProperty.cpp>\n"
            "#elif PLATFORM_STEAM_DECK\n"
            "#include <FunctionBodies/Platform/SteamDeck/5_01_MemberVariableLayout_DefaultSetter_FProperty.cpp>\n"
            "#else\n"
            "#include <FunctionBodies/5_01_MemberVariableLayout_DefaultSetter_FProperty.cpp>\n"
            "#endif";
        tests.expect(multi_platform_contents.contains(multi_platform_selection),
                     "distinct platforms should emit deterministic #if and #elif branches");

        auto duplicate_macro_layout = steam_deck_layout;
        duplicate_macro_layout.ifdef_macro = STR("PLATFORM_LINUX");
        config.platform_member_layouts = {duplicate_macro_layout, linux_layout};
        const auto duplicate_macro_message = exception_message([&] { generate_virtual_contents(); });
        tests.expect(duplicate_macro_message.contains("macro"),
                     "duplicate platform include macros should throw before emitting unreachable conditions");
        tests.expect(read_file(unreal_virtual_header) == multi_platform_header_contents,
                     "duplicate macro rejection should preserve the previously generated virtual header");
        tests.expect(read_file(unreal_virtual_file) == multi_platform_contents,
                     "duplicate macro rejection should preserve the previously generated virtual source");

        config.platform_member_layouts.clear();
        const auto zero_platform_contents = generate_virtual_contents();
        const std::string zero_platform_fallback =
            "#include <FunctionBodies/5_01_MemberVariableLayout_DefaultSetter_FProperty.cpp>";
        const auto fallback_position = zero_platform_contents.find(zero_platform_fallback);
        tests.expect(fallback_position != std::string::npos &&
                         fallback_position == zero_platform_contents.rfind(zero_platform_fallback),
                     "zero-platform selection should emit only one original fallback include");
        tests.expect(!zero_platform_contents.contains("#if ") &&
                         !zero_platform_contents.contains("#elif ") &&
                         !zero_platform_contents.contains("#else") &&
                         !zero_platform_contents.contains("#endif"),
                     "zero-platform selection should not emit any conditional wrapper");

        config.platform_member_layouts = {linux_layout};
        config.pdbs_to_dump = {std::filesystem::path{STR("5_01_CasePreserving.pdb")}};
        config.suffix_definitions.emplace(
            STR("CasePreserving"),
            SuffixDefinition{
                STR("CasePreserving"),
                STR("WITH_CASE_PRESERVING_NAME"),
                STR("Case-preserving name layout"),
                true,
            });
        tests.expect(generate_virtual_contents().contains(suffix_selection),
                     "preserved artifacts should retain the suffix-safe platform selection");

        if (artifacts.preserves_artifacts())
        {
            std::cout << "Preserved artifacts at " << artifacts.path().string() << '\n';
        }
        return tests.result();
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Unexpected test exception: " << exception.what() << '\n';
        return 1;
    }
}
