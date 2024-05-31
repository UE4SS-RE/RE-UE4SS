#pragma once

#include <variant>
#include <vector>

#include "Tokens.hpp"
#include <JSON/JSON.hpp>
#include <ParserBase/TokenParser.hpp>
// #include "Types.hpp"

#define RC_JSON_PARSER_SCOPED_ELEMENT_TYPES JSON::Object*, JSON::Array*

namespace RC::JSON::Parser
{
    class ScopeTypeVariant
    {
      private:
        std::variant<RC_JSON_PARSER_SCOPED_ELEMENT_TYPES> m_storage{};

      public:
        template <typename VariantType>
        ScopeTypeVariant(VariantType variant_type)
        {
            m_storage = std::move(variant_type);
        }

      public:
        template <typename GetType>
        auto as() const -> GetType&
        {
            return *std::get<GetType*>(m_storage);
        }
        template <typename GetType>
        auto as() -> GetType&
        {
            return *std::get<GetType*>(m_storage);
        }

        template <typename RequestedStoredType>
        auto is() const -> bool
        {
            return std::holds_alternative<RequestedStoredType*>(m_storage);
        }
        template <typename RequestedStoredType>
        auto is() -> bool
        {
            return std::holds_alternative<RequestedStoredType*>(m_storage);
        }
    };

    class ScopeStack
    {
      private:
        using StoredType = ScopeTypeVariant;

      private:
        std::vector<StoredType> m_storage{};

      public:
        auto push(StoredType) -> StoredType;
        auto pop() -> StoredType;
        auto peek_top() -> StoredType&;
        auto empty() -> bool
        {
            return m_storage.empty();
        }

        template <typename ValueType>
        auto add_element_to_top(std::unique_ptr<ValueType> value) -> ValueType*
        {
            auto& top = peek_top();
            if (top.is<JSON::Array>())
            {
                return top.as<JSON::Array>().get().emplace_back(std::move(value)).get();
            }
            else
            {
                throw std::runtime_error{"JSON::Parser::ScopeStack::add_element_to_top: Cannot add value-only element to non-array"};
            }
        }
        template <typename ValueType>
        auto add_element_to_top(StringType key, std::unique_ptr<ValueType> value) -> ValueType*
        {
            auto& top = peek_top();
            if (top.is<JSON::Object>())
            {
                return top.as<JSON::Object>().get().emplace(std::move(key), std::move(value)).first->second.get();
            }
            else
            {
                return top.as<JSON::Array>().get().emplace_back(std::move(value)).get();
            }
        }
    };

    class TokenParser : public ParserBase::TokenParser
    {
      private:
        enum class State
        {
            StartOfFile,
            ProcessNext,
            ReadKey,
            ReadValue,
        };

      private:
        std::unique_ptr<JSON::Object> m_global_object;
        ScopeStack m_scope_stack{};
        StringType m_last_key{};
        std::unique_ptr<JSON::Value> m_last_value{};
        State m_current_state{State::StartOfFile};
        TokenType m_expected_token{};
        bool m_string_started{};
        bool m_defer_element_creation{};

      public:
        TokenParser(const ParserBase::Tokenizer& tokenizer, File::StringType& input) : ParserBase::TokenParser(tokenizer, input)
        {
        }
        virtual ~TokenParser() = default;

      private:
        auto do_comma_verification() -> void;
        auto check_and_peek_next_tokens(TokenType type_to_check_against) -> bool;
        auto check_and_consume_next_tokens(const ParserBase::Token& token_in) -> bool;

      private:
        auto handle_opening_curly_brace_token(const ParserBase::Token&) -> void;
        auto handle_closing_curly_brace_token(const ParserBase::Token&) -> void;
        auto handle_opening_square_bracket_token(const ParserBase::Token&) -> void;
        auto handle_closing_square_bracket_token(const ParserBase::Token&) -> void;
        auto handle_double_quote_token(const ParserBase::Token&) -> void;
        auto handle_characters_token(const ParserBase::Token&) -> void;
        auto handle_comma_token(const ParserBase::Token&) -> void;
        auto handle_colon_token(const ParserBase::Token&) -> void;
        auto handle_boolean_token(const ParserBase::Token&, bool is_true) -> void;

      protected:
        auto parse_token(const ParserBase::Token&) -> void override;

      public:
        auto release_global_object() -> std::unique_ptr<JSON::Object>
        {
            return std::move(m_global_object);
        }
    };

} // namespace RC::JSON::Parser
