#include <UVTD/TypeContainer.hpp>

namespace RC::UVTD
{
    auto TypeContainer::join(const TypeContainer& other) -> void
    {
        class_entries.insert(other.class_entries.begin(), other.class_entries.end());
    }

    auto TypeContainer::get_or_create_class_entry(const File::StringType& symbol_name, const File::StringType& symbol_name_clean, const SymbolNameInfo& name_info) -> Class&
    {
        auto& class_entry = [&]() -> auto& {
            if (auto it = class_entries.find(symbol_name); it != class_entries.end())
            {
                return it->second;
            }
            else
            {
                return class_entries.emplace(symbol_name_clean, Class{
                                            .class_name = File::StringType{symbol_name},
                                            .class_name_clean = symbol_name_clean
                    }).first->second;
            }
        }();

        class_entry.valid_for_member_vars = name_info.valid_for_member_vars;
        class_entry.valid_for_vtable = name_info.valid_for_vtable;
        return class_entry;
    }
}