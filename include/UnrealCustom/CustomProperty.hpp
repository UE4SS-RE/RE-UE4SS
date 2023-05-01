#ifndef UE4SS_CUSTOMPROPERTY_HPP
#define UE4SS_CUSTOMPROPERTY_HPP

#include <cstdint>
#include <memory>

#include <Helpers/Casting.hpp>
#include <Unreal/FProperty.hpp>

#pragma warning(disable: 4068)

namespace RC::Unreal
{
    class UClass;
    class UScriptStruct;

    // Special base to be used for creating our own FProperty objects with our own custom data
    // Used when creating custom properties
    class CustomProperty : public FProperty
    {
    private:
        // Untyped data
        // The proper data will be copied inside at the proper offsets using StaticOffsetFinder
        #pragma clang diagnostic push
        #pragma clang diagnostic ignored "-Wunused-private-field"
        std::byte m_data[0x128]{};
        #pragma clang diagnostic pop

    public:
        CustomProperty(int32_t offset_internal, int32_t element_size);

    public:
        auto static construct(int32_t offset_internal, UClass* belongs_to_class, UClass* inner_class, int32_t element_size) -> std::unique_ptr<CustomProperty>;

        // 'set_member_variable' is not used anymore because we're using a completely different system for retrieving member variable offsets.
        // This system is abstracted more and doesn't expose offsets as easily.
        // Keeping this code here for reference in case it's needed for fixing things later.
        /*
        template<typename ValueType>
        auto static set_member_variable(const ::RC::Unreal::StaticOffsetFinder::MemberOffsets member, CustomProperty* property, ValueType new_value) -> void
        {
            auto* data_offset_internal = Helper::Casting::ptr_cast<ValueType*>(&property->m_data[StaticOffsetFinder::retrieve_static_offset(member)]);
            *data_offset_internal = new_value;
        }

        template<typename ValueType>
        auto static set_member_variable(const ::RC::Unreal::StaticOffsetFinder::MemberOffsets member, int32_t offset_offset, CustomProperty* property, ValueType new_value) -> void
        {
            auto* data_offset_internal = Helper::Casting::ptr_cast<ValueType*>(&property->m_data[StaticOffsetFinder::retrieve_static_offset(member) + offset_offset]);
            *data_offset_internal = new_value;
        }
        //*/
    };

    class CustomArrayProperty : public CustomProperty
    {
    public:
        CustomArrayProperty(int32_t offset_internal, int32_t element_size);

    public:
        // Used for the C++ API
        auto static construct(int32_t offset_internal, FProperty* array_inner, int32_t element_size) -> std::unique_ptr<CustomProperty>;

        // Used for the Lua API
        auto static construct(int32_t offset_internal, UClass* belongs_to_class, UClass* inner_class, FProperty* array_inner, int32_t element_size) -> std::unique_ptr<CustomProperty>;
    };

    class CustomStructProperty : public CustomProperty
    {
    public:
        CustomStructProperty(int32_t offset_internal, int32_t element_size);

    public:
        auto static construct(int32_t offset_internal, UScriptStruct* script_struct, int32_t element_size) -> std::unique_ptr<CustomProperty>;
    };
}

#pragma warning(default: 4068)

#endif //UE4SS_CUSTOMPROPERTY_HPP
