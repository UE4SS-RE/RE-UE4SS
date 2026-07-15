#include <UVTD/PlatformMemberLayoutGenerator.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <exception>
#include <format>
#include <fstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>

#include <Helpers/String.hpp>

namespace
{
    auto ascii_case_fold(std::string value) -> std::string
    {
        for (char& character : value)
        {
            if (character >= 'A' && character <= 'Z')
            {
                character = static_cast<char>(character - 'A' + 'a');
            }
        }
        return value;
    }

    auto output_parent(const std::filesystem::path& output_root) -> std::filesystem::path
    {
        auto parent = output_root.parent_path();
        return parent.empty() ? std::filesystem::path{"."} : parent;
    }

    auto owned_sibling_name(
        const std::filesystem::path& output_root,
        std::string_view purpose) -> std::filesystem::path
    {
        static std::atomic_uint64_t sequence{};
        const auto timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
        return output_parent(output_root) /
               std::format(
                   ".{}.uvtd-platform-member-layout-{}-{}-{}",
                   RC::to_utf8_string(output_root.filename()),
                   purpose,
                   timestamp,
                   sequence.fetch_add(1, std::memory_order_relaxed));
    }

    auto create_owned_staging_directory(const std::filesystem::path& output_root) -> std::filesystem::path
    {
        for (size_t attempt = 0; attempt < 100; ++attempt)
        {
            const auto candidate = owned_sibling_name(output_root, "staging");
            std::error_code create_error;
            if (std::filesystem::create_directory(candidate, create_error))
            {
                return candidate;
            }
            if (create_error)
            {
                throw std::runtime_error{std::format(
                    "Failed to create platform member layout staging directory '{}': {}",
                    RC::to_utf8_string(candidate),
                    create_error.message())};
            }
        }

        throw std::runtime_error{"Failed to allocate a unique platform member layout staging directory"};
    }

    auto unused_owned_backup_path(const std::filesystem::path& output_root) -> std::filesystem::path
    {
        for (size_t attempt = 0; attempt < 100; ++attempt)
        {
            const auto candidate = owned_sibling_name(output_root, "backup");
            std::error_code exists_error;
            const bool exists = std::filesystem::exists(candidate, exists_error);
            if (exists_error)
            {
                throw std::runtime_error{std::format(
                    "Failed to inspect platform member layout backup path '{}': {}",
                    RC::to_utf8_string(candidate),
                    exists_error.message())};
            }
            if (!exists)
            {
                return candidate;
            }
        }

        throw std::runtime_error{"Failed to allocate a unique platform member layout backup path"};
    }

    auto remove_owned_path(const std::filesystem::path& path) -> std::error_code
    {
        std::error_code cleanup_error;
        std::filesystem::remove_all(path, cleanup_error);
        return cleanup_error;
    }

    auto cleanup_failure_detail(
        const std::filesystem::path& path,
        const std::error_code& cleanup_error) -> std::string
    {
        if (!cleanup_error)
        {
            return {};
        }
        return std::format(
            "; additionally failed to remove owned path '{}': {}",
            RC::to_utf8_string(path),
            cleanup_error.message());
    }
}

namespace RC::UVTD
{
    PlatformMemberLayoutGenerator::PlatformMemberLayoutGenerator(
        std::vector<PlatformMemberLayout> layouts,
        std::filesystem::path output_root)
        : m_layouts{std::move(layouts)}, m_output_root{std::move(output_root)}
    {
    }

    auto PlatformMemberLayoutGenerator::generate_files() const -> void
    {
        auto layouts = m_layouts;
        std::ranges::sort(layouts, [](const PlatformMemberLayout& left, const PlatformMemberLayout& right) {
            return std::tie(left.platform, left.version) < std::tie(right.platform, right.version);
        });

        struct PlannedFile
        {
            const PlatformMemberLayout* layout;
            const File::StringType* class_name;
            std::filesystem::path relative_path;
        };

        std::vector<PlannedFile> planned_files;
        std::unordered_map<std::string, std::string> normalized_paths;

        for (const auto& layout : layouts)
        {
            std::vector<const File::StringType*> class_names;
            class_names.reserve(layout.classes.size());
            for (const auto& [class_name, _] : layout.classes)
            {
                class_names.emplace_back(&class_name);
            }
            std::ranges::sort(class_names, {}, [](const File::StringType* class_name) -> const File::StringType& {
                return *class_name;
            });

            for (const auto* class_name : class_names)
            {
                const auto relative_path = std::filesystem::path{to_utf8_string(layout.platform)} /
                                           std::format(
                                               "{}_MemberVariableLayout_DefaultSetter_{}.cpp",
                                               to_utf8_string(layout.version),
                                               to_utf8_string(*class_name));
                const auto relative_path_string = relative_path.generic_string();
                const auto normalized_path = ascii_case_fold(relative_path_string);
                const auto [existing, inserted] = normalized_paths.emplace(normalized_path, relative_path_string);
                if (!inserted)
                {
                    throw std::runtime_error{std::format(
                        "Platform member layout output path collision between '{}' and '{}'",
                        existing->second,
                        relative_path_string)};
                }

                planned_files.emplace_back(&layout, class_name, std::move(relative_path));
            }
        }

        const auto parent = output_parent(m_output_root);
        std::error_code parent_error;
        std::filesystem::create_directories(parent, parent_error);
        if (parent_error)
        {
            throw std::runtime_error{std::format(
                "Failed to create platform member layout output parent '{}': {}",
                to_utf8_string(parent),
                parent_error.message())};
        }

        std::error_code exists_error;
        const bool output_exists = std::filesystem::exists(m_output_root, exists_error);
        if (exists_error)
        {
            throw std::runtime_error{std::format(
                "Failed to inspect platform member layout output root '{}': {}",
                to_utf8_string(m_output_root),
                exists_error.message())};
        }
        const auto backup_root = output_exists ? unused_owned_backup_path(m_output_root) : std::filesystem::path{};

        const auto staging_root = create_owned_staging_directory(m_output_root);
        try
        {
            for (const auto& planned_file : planned_files)
            {
                const auto file_path = staging_root / planned_file.relative_path;
                std::error_code directory_error;
                std::filesystem::create_directories(file_path.parent_path(), directory_error);
                if (directory_error)
                {
                    throw std::runtime_error{std::format(
                        "Failed to create platform member layout directory '{}': {}",
                        to_utf8_string(file_path.parent_path()),
                        directory_error.message())};
                }

                const auto class_name_utf8 = to_utf8_string(*planned_file.class_name);
                std::ofstream file{file_path, std::ios::binary | std::ios::trunc};
                if (!file)
                {
                    throw std::runtime_error{std::format(
                        "Failed to create platform member layout file '{}'", to_utf8_string(file_path))};
                }

                const auto& class_members = planned_file.layout->classes.at(*planned_file.class_name);
                std::vector<const File::StringType*> member_names;
                member_names.reserve(class_members.size());
                for (const auto& [member_name, _] : class_members)
                {
                    member_names.emplace_back(&member_name);
                }
                std::ranges::sort(member_names, {}, [](const File::StringType* member_name) -> const File::StringType& {
                    return *member_name;
                });

                for (size_t index = 0; index < member_names.size(); ++index)
                {
                    const auto& member_name = *member_names[index];
                    const auto member_name_utf8 = to_utf8_string(member_name);
                    file << std::format(
                        "if (auto it = {}::MemberOffsets.find(STR(\"{}\")); it == {}::MemberOffsets.end())\n"
                        "{{\n"
                        "    {}::MemberOffsets.emplace(STR(\"{}\"), 0x{:X});\n"
                        "}}\n",
                        class_name_utf8,
                        member_name_utf8,
                        class_name_utf8,
                        class_name_utf8,
                        member_name_utf8,
                        class_members.at(member_name));
                    if (index + 1 < member_names.size())
                    {
                        file << '\n';
                    }
                }

                file.close();
                if (file.fail())
                {
                    throw std::runtime_error{std::format(
                        "Failed to fully write platform member layout file '{}'", to_utf8_string(file_path))};
                }
            }
        }
        catch (...)
        {
            const auto original_exception = std::current_exception();
            const auto cleanup_error = remove_owned_path(staging_root);
            if (cleanup_error)
            {
                throw std::runtime_error{std::format(
                    "Failed to clean platform member layout staging directory '{}': {}",
                    to_utf8_string(staging_root),
                    cleanup_error.message())};
            }
            std::rethrow_exception(original_exception);
        }

        if (!output_exists)
        {
            std::error_code publish_error;
            std::filesystem::rename(staging_root, m_output_root, publish_error);
            if (publish_error)
            {
                const auto cleanup_error = remove_owned_path(staging_root);
                throw std::runtime_error{std::format(
                    "Failed to publish platform member layouts to '{}': {}{}",
                    to_utf8_string(m_output_root),
                    publish_error.message(),
                    cleanup_failure_detail(staging_root, cleanup_error))};
            }
            return;
        }

        std::error_code backup_error;
        std::filesystem::rename(m_output_root, backup_root, backup_error);
        if (backup_error)
        {
            const auto cleanup_error = remove_owned_path(staging_root);
            throw std::runtime_error{std::format(
                "Failed to move existing platform member layouts to backup '{}': {}{}",
                to_utf8_string(backup_root),
                backup_error.message(),
                cleanup_failure_detail(staging_root, cleanup_error))};
        }

        std::error_code publish_error;
        std::filesystem::rename(staging_root, m_output_root, publish_error);
        if (publish_error)
        {
            std::error_code rollback_error;
            std::filesystem::rename(backup_root, m_output_root, rollback_error);
            const auto cleanup_error = remove_owned_path(staging_root);
            const auto cleanup_detail = cleanup_failure_detail(staging_root, cleanup_error);
            if (rollback_error)
            {
                throw std::runtime_error{std::format(
                    "Failed to publish platform member layouts to '{}' ({}) and failed to restore backup '{}' ({}); "
                    "previous output remains at the owned backup path{}",
                    to_utf8_string(m_output_root),
                    publish_error.message(),
                    to_utf8_string(backup_root),
                    rollback_error.message(),
                    cleanup_detail)};
            }
            throw std::runtime_error{std::format(
                "Failed to publish platform member layouts to '{}'; previous output was restored: {}{}",
                to_utf8_string(m_output_root),
                publish_error.message(),
                cleanup_detail)};
        }

        const auto backup_cleanup_error = remove_owned_path(backup_root);
        if (backup_cleanup_error)
        {
            throw std::runtime_error{std::format(
                "Published platform member layouts but failed to remove owned backup '{}': {}",
                to_utf8_string(backup_root),
                backup_cleanup_error.message())};
        }
    }

    auto PlatformMemberLayoutGenerator::output_cleanup() const -> void
    {
        std::error_code cleanup_error;
        std::filesystem::remove_all(m_output_root, cleanup_error);
        if (cleanup_error)
        {
            throw std::runtime_error{std::format(
                "Failed to remove platform member layout output root '{}': {}",
                to_utf8_string(m_output_root),
                cleanup_error.message())};
        }
    }
}
