#pragma once

#include <array>
#include <unordered_map>

#include <ParserBase/TokenParser.hpp>
// NOTE: '#include <IniParser/Section.hpp>' does not work for some reason
#include "Section.hpp"
#include "Tokens.hpp"

namespace RC::Ini
{
    class Value;

    enum class State
    {
        StartOfFile,
        NewLineStarted,
        CreateNewOrSetCurrentSection,
        CreateSectionKey,
        StoreSectionKey,
        SetSectionValue,
    };

    auto state_to_string(State) -> SystemStringType;

    class TokenParser : public ParserBase::TokenParser
    {
      private:
        using SectionContainer = std::unordered_map<SystemStringType, Section>;
        SectionContainer& m_output;
        Section* m_current_section{};
        Value* m_current_value{};
        SystemStringType m_current_character_data{};
        State m_current_state{State::StartOfFile};

      public:
        TokenParser(const ParserBase::Tokenizer& tokenizer, SystemStringType& input, std::unordered_map<SystemStringType, Section>& output)
            : ParserBase::TokenParser(tokenizer, input), m_output(output)
        {
        }

      public:
        RC_INI_PARSER_API auto static find_variable_by_name(Section* section, const SystemStringType& name) -> std::optional<std::reference_wrapper<Value>>;

      private:
        RC_INI_PARSER_API auto find_variable_by_name(const SystemStringType& name) -> std::optional<std::reference_wrapper<Value>>;
        RC_INI_PARSER_API auto characters_to_string(const ParserBase::Token& characters_token) -> SystemStringType;
        RC_INI_PARSER_API auto handle_opening_square_bracket_token(const ParserBase::Token& token) -> void;
        RC_INI_PARSER_API auto handle_closing_square_bracket_token(const ParserBase::Token& token) -> void;
        RC_INI_PARSER_API auto handle_space_token(const ParserBase::Token& token) -> void;
        RC_INI_PARSER_API auto handle_characters_token(const ParserBase::Token& token) -> void;
        RC_INI_PARSER_API auto handle_equals_token(const ParserBase::Token& token) -> void;
        RC_INI_PARSER_API auto handle_new_line_token(const ParserBase::Token& token) -> void;
        RC_INI_PARSER_API auto handle_semi_colon_token(const ParserBase::Token& token) -> void;

      protected:
        RC_INI_PARSER_API auto parse_token(const ParserBase::Token& token) -> void override;
    };
} // namespace RC::Ini
