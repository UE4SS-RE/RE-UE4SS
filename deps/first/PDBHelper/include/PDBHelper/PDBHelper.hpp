#pragma once

#include <filesystem>
#include <functional>
#include <vector>
#include <unordered_map>

#include <PDBHelper/Common.hpp>
#include <String/StringType.hpp>
#include <File/File.hpp>
#include <Helpers/UETargetModules.hpp>
#include <Constructs/Loop.hpp>

#include <PDB_RawFile.h>
#include <PDB_DBIStream.h>
#include <PDB_IPIStream.h>

namespace RC::PDB
{
    // Class that holds a single PDB file.
    // Ready to be queried only if 'is_valid' returns true.
    // All held data is automatically released when destructed.
    class RC_PDB_API SymbolsHolder
    {
    private:
        File::Handle m_pdb_file_handle{};
        std::span<uint8_t> m_pdb_memory_map{};
        std::unique_ptr<::PDB::RawFile> m_pdb_file_raw{};
        ::PDB::IPIStream m_ipi_stream{};
        ::PDB::DBIStream m_dbi_stream{};
        ::PDB::ImageSectionStream m_image_section_stream{};
        ::PDB::CoalescedMSFStream m_symbol_record_stream{};
        ::PDB::PublicSymbolStream m_public_symbol_stream{};
        const ScanTargetArray* m_modules_array{};
        bool m_is_valid{};

    public:
        SymbolsHolder();
        SymbolsHolder(const std::filesystem::path& pdb_file, const ScanTargetArray* modules_array);

    public:
        [[nodiscard]] auto is_valid() const -> bool
        {
            return m_is_valid;
        }

        // Returns the RVA for the supplied symbol, or 0 if the symbol isn't found.
        [[nodiscard]] auto find_symbol_rva(const StringType& symbol_name) -> uint32_t;

        // Returns the address for the supplied symbol, or nullptr if the symbol isn't found.
        [[nodiscard]] auto get_address_of_symbol(const StringType& symbol_name, ScanTarget scan_target) -> uintptr_t;

        // Iterates each symbol and calls the supplied callback.
        // The callback can LoopAction::Break to exit early.
        using CallbackType = std::function<LoopAction(std::string_view symbol_name, uint32_t rva)>;
        auto for_each_symbol(const CallbackType&) -> void;

    private:
        [[nodiscard]] auto HasValidDBIStreams() const -> bool;

        // This function should remain private to avoid accidentally initializing twice.
        // If you need to reinitialize, re-assign the variable, i.e.: holder = SymbolsHolder("path-to-pdb")
        // This will ensure that any old data is correctly dealt with before initialization happens again.
        auto initialize() -> void;

    private:
        [[nodiscard]] static auto IsError(::PDB::ErrorCode error_code) -> bool;
    };

    // Helper that initializes instances of SymbolsHolder on demand, and exposes helper functions that act on all of them.
    class RC_PDB_API SymbolsHelper
    {
    private:
        std::array<SymbolsHolder, static_cast<size_t>(ScanTarget::Max)> m_symbol_holders{};
        std::filesystem::path m_exe_path{};
        const ScanTargetArray* m_modules_array{};
        bool m_is_modular{};
        using CallbackType = std::function<void(uintptr_t)>;
        std::unordered_map<ScanTarget, std::vector<std::tuple<std::string, CallbackType, bool>>> m_callbacks{};

    public:
        SymbolsHelper() = delete;
        SymbolsHelper(std::filesystem::path exe_path, const ScanTargetArray* modules_array, bool is_modular);

    public:
        // Returns the address for the supplied symbol, or nullptr if the symbol isn't found.
        // Only recommended if you're looking for a single symbol.
        // If you have many symbols to lookup, use 'register_symbol_for_scan' and 'start_scan' instead to do a batch lookup.
        [[nodiscard]] auto get_address_of_symbol(const StringType& symbol_name, ScanTarget scan_target) -> uintptr_t;

        // Returns a vector of all symbols in all modules containing substrs in their name.
        [[nodiscard]] auto find_all_symbols_containing_substr(const std::vector<std::string_view>& substrs) -> std::vector<std::pair<std::string, std::string>>;

        // Register a callback for when a symbol is found with the supplied name.
        auto add_to_batch(std::string symbol_name, ScanTarget scan_target, const CallbackType& callback) -> void;

        // Starts scanning symbols.
        auto start_batch_lookup() -> void;

    private:
        auto get_symbol_holder(ScanTarget) -> SymbolsHolder*;

    public:
        [[nodiscard]] static auto get_pdb_file_name_for_target(ScanTarget scan_target) -> std::filesystem::path;
    };
} // namespace RC::PDB
