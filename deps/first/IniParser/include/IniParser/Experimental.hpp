#pragma once

#include <ProtoParser/Token.hpp>
#include <ProtoParser/TokenParser.hpp>
#include <ProtoParser/Tokenizer.hpp>
#include <String/StringType.hpp>

#include <String/StringType.hpp>

namespace RC::Parser::Experimental
{
    enum TokenType : int
    {
        EndOfFile = -1, // Special token, value of -1 is always EOF

        CarriageReturn,
        NewLine,
        Space,
        Characters,
        Equals,
        Plus,
        ClosingSquareBracket,
        OpeningSquareBracket,
        SemiColon,
    };

    class RuleOne : public Parser::TokenRule
    {
      public:
        [[nodiscard]] auto to_string() const -> StringType override
        {
            return STR("RuleOne");
        }
    };

    class TokenMustEndWithOppositeToken : public Parser::TokenRule
    {
      public:
        TokenMustEndWithOppositeToken() : TokenRule(STR("TokenMustEndWithOppositeToken"))
        {
        }

        auto exec(const Parser::Token& token, const CharType* start_of_token, size_t current_cursor_location, Tokenizer& tokenizer) -> int override
        {
            printf_s("TokenMustEndWithOppositeToken::exec [%S]\n", token.to_string().c_str());
            return 0;
        }

        [[nodiscard]] auto to_string() const -> StringType override
        {
            return STR("TokenMustEndWithOppositeToken");
        }
    };

    class TokenMustHaveCharsBeforeEnd : public Parser::TokenRule
    {
      public:
        TokenMustHaveCharsBeforeEnd() : TokenRule(STR("TokenMustHaveCharsBeforeEnd"))
        {
        }

        auto exec(const Parser::Token& token, const CharType* start_of_token, size_t current_cursor_location, Tokenizer& tokenizer) -> int override
        {
            printf_s("TokenMustHaveCharsBeforeEnd::exec [%S]\n", token.to_string().c_str());
            return 0;
        }

        [[nodiscard]] auto to_string() const -> StringType override
        {
            return STR("TokenMustHaveCharsBeforeEnd");
        }
    };

    class ExperimentalTokenParser : public Parser::TokenParser
    {
      public:
        struct Value
        {
            // Always use "ref->value" instead of "value"
            // That way it returns properly if this value is a refernece to another variable
            // The "ref->value" member is set to self if it doesn't refer to another variable
            StringType value{};
            const Value* ref{};
        };

        struct Section
        {
            std::unordered_map<StringType, Value> key_value_pairs{};
        };

      private:
        using SectionContainer = std::unordered_map<StringType, Section>;
        SectionContainer& m_output;
        Section* m_current_section{};

        Value* m_variable_to_assign_to{};
        StringType m_temporary{};

      public:
        ExperimentalTokenParser(const Parser::Tokenizer& tokenizer, StringType input, std::unordered_map<StringType, Section>& output)
            : TokenParser(tokenizer, input), m_output(output)
        {
        }

      public:
        auto static find_variable_by_name(Section* section, const StringType& name) -> std::optional<std::reference_wrapper<Value>>;
        auto static find_variable_by_name(SectionContainer& sections, const StringType& name) -> std::optional<std::reference_wrapper<Value>>;

      private:
        auto find_variable_by_name(const StringType& name) -> std::optional<std::reference_wrapper<Value>>;
        auto try_set_section_value(Value& pair_value, const Parser::Token& token, bool is_space_valid = true) -> void;
        auto handle_operator_equals(const Parser::Token& token) -> void;
        auto handle_operator_plus(const Parser::Token& token) -> void;
        auto handle_opening_square_bracket(const Parser::Token& token) -> void;
        auto handle_single_line_comment() -> void;

      protected:
        auto parse_token(const Parser::Token& token) -> void override;

      public:
    };

    class ExperimentalParser
    {
      private:
        std::unordered_map<StringType, ExperimentalTokenParser::Section> m_sections;

      public:
        ExperimentalParser(const StringType& input);

      private:
        auto create_available_tokens_for_tokenizer() -> Parser::TokenContainer;

      public:
        auto get_string(const StringType& section, const StringType& key, const StringType& default_value) -> StringType;
    };

    auto test() -> void;
} // namespace RC::Parser::Experimental
