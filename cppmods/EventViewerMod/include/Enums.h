#pragma once

namespace RC::EventViewerMod
{
#define EVM_MIDDLEWARE_HOOK_TARGET_FLAGS(X, EnumName) \
    X(EnumName, All, ( (1 << 0) | (1 << 1) | (1 << 2) )) \
    X(EnumName, ProcessEvent, (1 << 0))                   \
    X(EnumName, ProcessInternal, (1 << 1))                \
    X(EnumName, ProcessLocalScriptFunction, (1 << 2))

#define EVM_MODE(X, EnumName) \
    X(EnumName, Stack)        \
    X(EnumName, Frequency)

#define EVM_STRINGIZE_1(x) #x
#define EVM_STRINGIZE(x)   EVM_STRINGIZE_1(x)

// Per-item emitters
#define EVM_ENUM_ELEM(EnumName, v)  v,
#define EVM_ENUM_CASE(EnumName, v)  case E##EnumName::v: return EVM_STRINGIZE(v);
#define EVM_ENUM_STR(EnumName, v)   EVM_STRINGIZE(v),
#define EVM_ENUM_COUNT(EnumName, v) +1

// Per-item emitters (with values)
#define EVM_ENUM_ELEM_V(EnumName, v, val)  v = val,
#define EVM_ENUM_CASE_V(EnumName, v, val)  case E##EnumName::v: return EVM_STRINGIZE(v);
#define EVM_ENUM_STR_V(EnumName, v, val)   EVM_STRINGIZE(v),
#define EVM_ENUM_VAL_V(EnumName, v, val)   E##EnumName::v,
#define EVM_ENUM_COUNT_V(EnumName, v, val) +1

#define EVM_DECLARE_REFLECTED_ENUM(Name, LIST2)                         \
    enum class E##Name : int                                             \
    {                                                                    \
        LIST2(EVM_ENUM_ELEM, Name)                                       \
    };                                                                   \
                                                                         \
    inline static constexpr int E##Name##_Size =                         \
        0 LIST2(EVM_ENUM_COUNT, Name);                                   \
                                                                         \
    inline static constexpr const char* E##Name##_NameArray[E##Name##_Size] = \
    {                                                                    \
        LIST2(EVM_ENUM_STR, Name)                                        \
    };                                                                   \
                                                                         \
    inline constexpr const char* to_string(E##Name e) noexcept           \
    {                                                                    \
        switch (e)                                                       \
        {                                                                \
            LIST2(EVM_ENUM_CASE, Name)                                   \
            default: return "<unknown " #Name ">";                       \
        }                                                                \
    }

#define EVM_DECLARE_REFLECTED_ENUM_VALUES(Name, LIST3)                   \
    enum class E##Name : int                                             \
    {                                                                    \
        LIST3(EVM_ENUM_ELEM_V, Name)                                     \
    };                                                                   \
                                                                         \
    inline static constexpr int E##Name##_Size =                         \
        0 LIST3(EVM_ENUM_COUNT_V, Name);                                 \
                                                                         \
    inline static constexpr E##Name E##Name##_ValueArray[E##Name##_Size] = \
    {                                                                    \
        LIST3(EVM_ENUM_VAL_V, Name)                                      \
    };                                                                   \
                                                                         \
    inline static constexpr const char* E##Name##_NameArray[E##Name##_Size] = \
    {                                                                    \
        LIST3(EVM_ENUM_STR_V, Name)                                      \
    };                                                                   \
                                                                         \
    inline constexpr const char* to_string(E##Name e) noexcept           \
    {                                                                    \
        switch (e)                                                       \
        {                                                                \
            LIST3(EVM_ENUM_CASE_V, Name)                                 \
            default: return "<unknown " #Name ">";                       \
        }                                                                \
    }

EVM_DECLARE_REFLECTED_ENUM_VALUES(MiddlewareHookTarget, EVM_MIDDLEWARE_HOOK_TARGET_FLAGS);
EVM_DECLARE_REFLECTED_ENUM(Mode, EVM_MODE);

#undef EVM_DECLARE_REFLECTED_ENUM_VALUES
#undef EVM_DECLARE_REFLECTED_ENUM
#undef EVM_ENUM_COUNT_V
#undef EVM_ENUM_VAL_V
#undef EVM_ENUM_STR_V
#undef EVM_ENUM_CASE_V
#undef EVM_ENUM_ELEM_V
#undef EVM_ENUM_COUNT
#undef EVM_ENUM_STR
#undef EVM_ENUM_CASE
#undef EVM_ENUM_ELEM
#undef EVM_STRINGIZE
#undef EVM_STRINGIZE_1
#undef EVM_MODE
#undef EVM_MIDDLEWARE_HOOK_TARGET_FLAGS

} // namespace RC::EventViewerMod
