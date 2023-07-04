#ifndef UNREALVTABLEDUMPER_TYPECONTAINER_HPP
#define UNREALVTABLEDUMPER_TYPECONTAINER_HPP

#include <unordered_map>

#include <UVTD/Symbols.hpp>
#include <File/File.hpp>

namespace RC::UVTD
{
    class TypeContainer
    {
    private:
        using ClassEntries = std::unordered_map<File::StringType, Class>;

        ClassEntries class_entries;

    public:
        auto join(const TypeContainer& other) -> void;

    public:
        constexpr auto get_class_entries() const -> const ClassEntries& { return class_entries; }

    public:
        auto get_or_create_class_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean, const SymbolNameInfo& name_info) -> Class&;
    };
}

#endif