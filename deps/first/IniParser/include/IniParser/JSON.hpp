#pragma once

#include <memory>
#include <vector>

#include <File/File.hpp>
#include <ProtoParser/TokenParser.hpp>

#include <File/Macros.hpp>

namespace RC::Parser
{
    namespace JSONInternal
    {
        enum TokenType : int
        {
            Default = -1,
            OpenCurlyBrace,
            CloseCurlyBrace,
            OpenSquareBracket,
            CloseSquareBracket,
            Colon,
            DoubleQuote,
            Comma,
            Characters,
            EndOfFile,
        };

        auto static token_type_to_string(const TokenType token_type) -> std::string
        {
            switch (token_type)
            {
            case Default:
                return std::string("Default");
            case OpenCurlyBrace:
                return std::string("OpenCurlyBrace");
            case CloseCurlyBrace:
                return std::string("CloseCurlyBrace");
            case OpenSquareBracket:
                return std::string("OpenSquareBracket");
            case CloseSquareBracket:
                return std::string("CloseSquareBracket");
            case Colon:
                return std::string("Colon");
            case Comma:
                return std::string("Comma");
                break;
            case DoubleQuote:
                return std::string("DoubleQuote");
            case Characters:
                return std::string("Characters");
            case EndOfFile:
                return std::string("EndOfFile");
            }
        }

        enum ItemType
        {
            None,
            String,
            Object,
            Array,
        };

        class ItemBase
        {
          public:
            UEStringType m_name{STR("--UNNAMED-ITEM--")};
            bool m_is_global_scope{false};

          public:
            auto get_name() -> SystemStringType;

            virtual ~ItemBase() = default;
            virtual auto to_string() -> SystemStringType = 0;
            virtual auto get_type() -> ItemType = 0;
        };

        class StringItem : public ItemBase
        {
          public:
            SystemStringType m_value{};

          public:
            StringItem() = default;
            StringItem(const SystemStringType& value) : m_value(value)
            {
            }

            auto to_string() -> SystemStringType override;
            auto get_type() -> ItemType override
            {
                return ItemType::String;
            }
        };

        class ObjectScope : public ItemBase
        {
          public:
            std::vector<std::unique_ptr<ItemBase>> m_members{};
            size_t m_previous_line_without_comma{};
            size_t m_previous_column_without_comma{};

          public:
            auto to_string() -> SystemStringType override;
            auto get_type() -> ItemType override
            {
                return ItemType::Object;
            }
        };

        class ArrayScope : public ItemBase
        {
          public:
            std::vector<std::unique_ptr<ItemBase>> m_members{};

          public:
            auto to_string() -> SystemStringType override;
            auto get_type() -> ItemType override
            {
                return ItemType::Array;
            }
        };

        class TokenTypeStack
        {
          private:
            std::vector<TokenType> m_token_types{};

          public:
            auto operator+=(const TokenType& rhs)
            {
                m_token_types.emplace_back(rhs);
            }

            auto operator[](int32_t relative_index_to_front)
            {
                int32_t real_index = (m_token_types.size() - 1) + relative_index_to_front;

                if (real_index < 0)
                {
                    return TokenType::Default;
                }
                if (real_index > m_token_types.size())
                {
                    throw std::runtime_error{"[TokenTypeStack] Stack accessed (+) out-of-bounds"};
                }

                return m_token_types[real_index];
            }

          public:
        };

        class TokenParser : public Parser::TokenParser
        {
          private:
            std::vector<std::unique_ptr<ItemBase>> m_items{};

            // Non-owning
            ItemBase* m_current_item{};
            ItemType m_next_item_expected{};
            TokenType m_next_token_expected{TokenType::Default};
            TokenTypeStack m_processed_token_types{};
            SystemStringType m_string_value_buffer{};

            bool m_double_quote_opened{false};
            bool m_double_quote_successfully_closed{false};

          public:
            TokenParser(const Parser::Tokenizer& tokenizer, SystemStringType& input) : Parser::TokenParser(tokenizer, input)
            {
            }

          private:
            auto token_to_string(const Token& token) -> SystemStringType;
            auto skip_all_spaces() -> void;

            auto parse_open_curly_brace(const Token& token) -> void;
            auto parse_close_curly_brace(const Token& token) -> void;
            auto parse_colon(const Token& token) -> void;
            auto parse_comma(const Token& token) -> void;
            auto parse_double_quote(const Token& token) -> void;
            auto parse_characters(const Token& token) -> void;

          public:
            auto release_contents() -> std::vector<std::unique_ptr<ItemBase>>;

          protected:
            auto parse_token(const Parser::Token& token) -> void override;
        };
    } // namespace JSONInternal

    class JSON
    {
      private:
        std::unique_ptr<JSONInternal::TokenParser> m_token_parser{};

      public:
        JSON() = default;

      private:
        auto parse_internal(SystemStringType& input) -> void;

      public:
        auto parse(SystemStringType& input) -> void;
        auto parse(File::Handle&) -> void;
        auto release_contents() -> std::vector<std::unique_ptr<JSONInternal::ItemBase>>;

      public:
        auto static test() -> void;
    };
} // namespace RC::Parser
