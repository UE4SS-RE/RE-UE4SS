#pragma once

#include <functional>
#include <unordered_map>

#include <File/File.hpp>

namespace RC::ObjectDumper
{
    using ToStringHash = size_t;
    using ObjectToStringDecl = std::function<void(void*, SystemStringType&)>;
    extern std::unordered_map<ToStringHash, ObjectToStringDecl> object_to_string_functions;

    using ObjectToStringComplexDeclCallable = const std::function<void(void*)>&;
    using ObjectToStringComplexDecl = std::function<void(void*, SystemStringType&, ObjectToStringComplexDeclCallable)>;
    extern std::unordered_map<ToStringHash, ObjectToStringComplexDecl> object_to_string_complex_functions;

    auto get_to_string(size_t hash) -> ObjectToStringDecl;
    auto get_to_string_complex(size_t hash) -> ObjectToStringComplexDecl;
    auto to_string_exists(size_t hash) -> bool;
    auto to_string_complex_exists(size_t hash) -> bool;

    auto object_trivial_dump_to_string(void* p_this, SystemStringType& out_line, const SystemCharType* post_delimiter = SYSSTR(".")) -> void;
    auto object_to_string(void* p_this, SystemStringType& out_line) -> void;

    auto property_trivial_dump_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto property_to_string(void* p_this, SystemStringType& out_line) -> void;

    auto arrayproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto arrayproperty_to_string_complex(void* p_this, SystemStringType& out_line, ObjectToStringComplexDeclCallable callable) -> void;

    auto mapproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto mapproperty_to_string_complex(void* p_this, SystemStringType& out_line, ObjectToStringComplexDeclCallable callable) -> void;

    auto classproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto delegateproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto fieldpathproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto interfaceproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto multicastdelegateproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto objectproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto structproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto enumproperty_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto boolproperty_to_string(void* p_this, SystemStringType& out_line) -> void;

    auto enum_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto struct_to_string(void* p_this, SystemStringType& out_line) -> void;
    auto function_to_string(void* p_this, SystemStringType& out_line) -> void;

    auto scriptstruct_to_string_complex(void* p_this, SystemStringType& out_line, ObjectToStringComplexDeclCallable callable) -> void;

    auto init() -> void;
} // namespace RC::ObjectDumper
