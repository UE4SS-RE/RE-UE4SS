#include <codecvt>
#include <fstream>
#include <File/File.hpp>

namespace RC::File
{
    auto construct_handle(const std::filesystem::path& file_name, const OpenProperties& open_properties) -> Handle
    {
        auto internal_handle = Handle::FileType::open_file(file_name, open_properties);
        return Handle{std::move(internal_handle)};
    }

    auto open(const std::filesystem::path& file_path_and_name,
              OpenFor open_for,
              OverwriteExistingFile overwrite_existing_file,
              CreateIfNonExistent create_if_non_existent) -> Handle
    {
        OpenProperties open_properties{
                .open_for = open_for,
                .overwrite_existing_file = overwrite_existing_file,
                .create_if_non_existent = create_if_non_existent,
        };

        return construct_handle(file_path_and_name, open_properties);
    }

    // BOM sequences
    const std::string BOM_UTF8 = "\xEF\xBB\xBF";
    const std::wstring BOM_UTF16_LE = L"\xFEFF";
    const std::wstring BOM_UTF16_BE = L"\xFFFE";

    // Function to check for BOM and reopen the file as a wide stream
    auto open_file_skip_BOM(const std::wstring& filename) -> StreamType
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open())
        {
            throw std::runtime_error("Failed to open the file.");
        }

        char bom[3] = {0};
        file.read(bom, 3);
        if (std::string(bom, 3) == BOM_UTF8)
        {
            // UTF-8 BOM detected
            file.close();
            StreamType wfile(filename);
            wfile.seekg(3, std::ios::beg); // Skip BOM
            return wfile;
        }
        else
        {
            // Check for wide BOM
            wchar_t wbom;
            file.seekg(0, std::ios::beg); // Rewind to start
            file.read(reinterpret_cast<char*>(&wbom), sizeof(wchar_t));
            file.close();
            if (wbom == BOM_UTF16_LE[0] || wbom == BOM_UTF16_BE[0])
            {
                StreamType wfile(filename);
#pragma warning(disable : 4996)
                std::locale loc(wfile.getloc(), new std::codecvt_utf8_utf16<wchar_t>);
#pragma warning(default : 4996)
                wfile.imbue(loc);
                wfile.seekg(sizeof(wchar_t), std::ios::beg); // Skip BOM
                return wfile;
            }
            else
            {
                // No BOM detected
                file.close();
                return StreamType(filename);
            }
        }
    }

    auto delete_file(const std::filesystem::path& file_path_and_name) -> void
    {
        Handle::FileType::delete_file(file_path_and_name);
    }
} // namespace RC::File
