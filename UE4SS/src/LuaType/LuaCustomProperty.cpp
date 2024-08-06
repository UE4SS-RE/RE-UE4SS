#include <bit>

#include <LuaType/LuaCustomProperty.hpp>
#pragma warning(disable : 4005)
#include <Unreal/FProperty.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UnrealVersion.hpp>
#include <UnrealCustom/CustomProperty.hpp>
#pragma warning(default : 4005)

namespace RC::LuaType
{
    LuaCustomProperty::PropertyList LuaCustomProperty::StaticStorage::property_list;

    LuaCustomProperty::LuaCustomProperty(StringType name, std::unique_ptr<Unreal::CustomProperty> property) : m_name(name), m_property(std::move(property))
    {
    }

    auto LuaCustomProperty::PropertyList::add(StringType property_name, std::unique_ptr<Unreal::CustomProperty> property) -> void
    {
        (void)properties.emplace_back(LuaCustomProperty{property_name, std::move(property)}).m_property.get();
    }

    auto LuaCustomProperty::PropertyList::clear() -> void
    {
        properties.clear();
    }

    auto LuaCustomProperty::PropertyList::find_or_nullptr(Unreal::UObject* base, StringType property_name) -> Unreal::FProperty*
    {
        Unreal::FProperty* custom_property_found{};

        for (const auto& property_item : properties)
        {
            void* owner_or_outer{};
            auto owner = property_item.m_property->GetOwnerVariant();
            if (owner.IsUObject())
            {
                owner_or_outer = owner.ToUObject();
            }
            else
            {
                owner_or_outer = owner.ToField();
            }

            Unreal::UStruct* ptr = base->GetClassPrivate();
            bool class_matches = ptr == owner_or_outer;

            if (!class_matches)
            {
                for (Unreal::UStruct* super_struct : ptr->ForEachSuperStruct())
                {
                    if (super_struct == owner_or_outer)
                    {
                        class_matches = true;
                        break;
                    }
                }
            }

            if (class_matches && property_name == property_item.m_name)
            {
                // Compare name here
                custom_property_found = property_item.m_property.get();
                break;
            }
        }

        return custom_property_found;
    }
} // namespace RC::LuaType
