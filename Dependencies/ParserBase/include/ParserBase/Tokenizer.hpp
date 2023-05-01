#ifndef RC_PARSER_TOKENIZER_HPP
#define RC_PARSER_TOKENIZER_HPP

#include <vector>
#include <string>
#include <memory>
#include <stdexcept>

#include <ParserBase/Common.hpp>
#include <File/Macros.hpp>

namespace RC::ParserBase
{
    class Token;

    template<typename SupposedEnum>
    concept EnumType = std::is_enum_v<SupposedEnum> && std::is_same_v<std::underlying_type_t<SupposedEnum>, int>;

    class TokenContainer
    {
    private:
        friend class Tokenizer;

    private:
        std::vector<Token> m_tokens;
        int m_eof_token_type;
        bool m_has_eof_token_type{};

    public:
        RC_PB_API auto add(Token token) -> size_t;
        RC_PB_API auto set_eof_token(int token_type) -> void;
        RC_PB_API auto get_all() -> const std::vector<Token>&;
        RC_PB_API auto get_by_type(int type) -> Token*;
    };

    class Tokenizer
    {
    public:
        TokenContainer m_token_container;
        std::vector<Token> m_tokens_in_input;
        size_t m_current_line{0};
        size_t m_current_column{0};

    public:
        RC_PB_API auto set_available_tokens(TokenContainer&&) -> void;
        // TODO: Maybe the constructor should take the input instead of 'tokenize'
        RC_PB_API auto tokenize(const File::StringType& input) -> void;
        [[nodiscard]] RC_PB_API auto get_tokens() const -> const std::vector<Token>&;
        [[nodiscard]] RC_PB_API auto get_last_token() const -> const Token&;
    };
}

#endif //RC_PARSER_TOKENIZER_HPP
