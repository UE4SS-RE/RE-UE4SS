#pragma once

#include <unordered_map>

#include <File/File.hpp>
#include <IniParser/Common.hpp>
#include <IniParser/Section.hpp>
#include <ParserBase/Token.hpp>
#include <ParserBase/TokenParser.hpp>
#include <ParserBase/Tokenizer.hpp>

namespace RC::Ini
{
    class Value;

    class Parser
    {
      public:
        enum class CanThrow
        {
            Yes,
            No,
        };

      private:
        std::unordered_map<SystemStringType, Section> m_sections;
        bool m_parsing_is_complete{false};

      public:
        Parser() = default;

      private:
        RC_INI_PARSER_API auto parse_internal(SystemStringType& input) -> void;
        RC_INI_PARSER_API auto create_available_tokens_for_tokenizer() -> ParserBase::TokenContainer;
        RC_INI_PARSER_API auto get_value(const SystemStringType& section, const SystemStringType& key, CanThrow = CanThrow::Yes) const
                -> std::optional<std::reference_wrapper<const Value>>;

      public:
        RC_INI_PARSER_API auto parse(SystemStringType& input) -> void;
        RC_INI_PARSER_API auto parse(const File::Handle&) -> void;
        RC_INI_PARSER_API auto get_list(const SystemStringType& section) -> List;
        RC_INI_PARSER_API auto get_ordered_list(const SystemStringType& section) -> List;
        RC_INI_PARSER_API auto get_string(const SystemStringType& section, const SystemStringType& key, const SystemStringType& default_value) const noexcept
                -> const SystemStringType&;
        RC_INI_PARSER_API auto get_string(const SystemStringType& section, const SystemStringType& key) const -> const SystemStringType&;
        // Should there be more integer getters ? They'd confirm size and convert explicitly (or throw?), no implicit conversions.
        RC_INI_PARSER_API auto get_int64(const SystemStringType& section, const SystemStringType& key, int64_t default_value) const noexcept -> int64_t;
        RC_INI_PARSER_API auto get_int64(const SystemStringType& section, const SystemStringType& key) const -> int64_t;
        RC_INI_PARSER_API auto get_float(const SystemStringType& section, const SystemStringType& key, float default_value) const noexcept -> float;
        RC_INI_PARSER_API auto get_float(const SystemStringType& section, const SystemStringType& key) const -> float;
        RC_INI_PARSER_API auto get_bool(const SystemStringType& section, const SystemStringType& key, bool default_value) const noexcept -> bool;
        RC_INI_PARSER_API auto get_bool(const SystemStringType& section, const SystemStringType& key) const -> bool;
    };
} // namespace RC::Ini
