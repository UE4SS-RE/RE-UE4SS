#ifndef RC_JSON_PARSER_HPP
#define RC_JSON_PARSER_HPP

#include <vector>
#include <memory>

#include <File/File.hpp>
#include <ProtoParser/TokenParser.hpp>

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

        auto static token_type_to_string(const TokenType token_type) -> File::StringType
        {
            switch (token_type)
            {
                case Default:
                    return STR("Default");
                case OpenCurlyBrace:
                    return STR("OpenCurlyBrace");
                case CloseCurlyBrace:
                    return STR("CloseCurlyBrace");
                case OpenSquareBracket:
                    return STR("OpenSquareBracket");
                case CloseSquareBracket:
                    return STR("CloseSquareBracket");
                case Colon:
                    return STR("Colon");
                case Comma:
                    return STR("Comma");
                    break;
                case DoubleQuote:
                    return STR("DoubleQuote");
                case Characters:
                    return STR("Characters");
                case EndOfFile:
                    return STR("EndOfFile");
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
            File::StringType m_name{STR("--UNNAMED-ITEM--")};
            bool m_is_global_scope{false};

        public:

            auto get_name() -> File::StringViewType;

            virtual ~ItemBase() = default;
            virtual auto to_string() -> File::StringType = 0;
            virtual auto get_type() -> ItemType = 0;
        };

        class StringItem : public ItemBase
        {
        public:
            File::StringType m_value{};

        public:
            StringItem() = default;
            StringItem(const File::StringType& value) : m_value(value) {}

            auto to_string() -> File::StringType override;
            auto get_type() -> ItemType override { return ItemType::String; }
        };

        class ObjectScope : public ItemBase
        {
        public:
            std::vector<std::unique_ptr<ItemBase>> m_members{};
            size_t m_previous_line_without_comma{};
            size_t m_previous_column_without_comma{};

        public:
            auto to_string() -> File::StringType override;
            auto get_type() -> ItemType override { return ItemType::Object; }
        };

        class ArrayScope : public ItemBase
        {
        public:
            std::vector<std::unique_ptr<ItemBase>> m_members{};

        public:
            auto to_string() -> File::StringType override;
            auto get_type() -> ItemType override { return ItemType::Array; }
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
            File::StringType m_string_value_buffer{};

            bool m_double_quote_opened{false};
            bool m_double_quote_successfully_closed{false};

        public:
            TokenParser(const Parser::Tokenizer& tokenizer, File::StringType& input) : Parser::TokenParser(tokenizer, input) {}

        private:
            auto token_to_string(const Token& token) -> File::StringType;
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
    }

    class JSON
    {
    private:
        std::unique_ptr<JSONInternal::TokenParser> m_token_parser{};

    public:
        JSON() = default;

    private:
        auto parse_internal(File::StringType& input) -> void;

    public:
        auto parse(File::StringType& input) -> void;
        auto parse(File::Handle&) -> void;
        auto release_contents() -> std::vector<std::unique_ptr<JSONInternal::ItemBase>>;

    public:
        auto static test() -> void;
    };
}

#endif //RC_JSON_PARSER_HPP
