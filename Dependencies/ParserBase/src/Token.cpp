#include <ParserBase/Token.hpp>

namespace RC::ParserBase
{
    Token::Token(int type, File::StringViewType name, File::StringViewType identifier, Token::HasData has_data)
            : m_type(type),
              m_debug_name(name),
              m_identifier(identifier),
              m_has_data(has_data)
    {
    }

    auto Token::get_type() const -> int
    {
        return m_type;
    }

    auto Token::set_has_data(HasData new_has_data) -> void
    {
        m_has_data = new_has_data;
    }

    auto Token::has_data() const -> bool
    {
        return m_has_data == HasData::Yes;
    }

    auto Token::set_start(size_t new_start) -> void
    {
        m_start = new_start;
    }

    auto Token::get_start() const -> size_t
    {
        return m_start;
    }

    auto Token::set_end(size_t new_end) -> void
    {
        m_end = new_end;
    }

    auto Token::get_end() const -> size_t
    {
        return m_end;
    }

    auto Token::get_identifier() const -> File::StringViewType
    {
        return m_identifier;
    }

    auto Token::get_line() const -> size_t
    {
        return m_line;
    }

    auto Token::get_column() const -> size_t
    {
        return m_column;
    }

    auto Token::get_rules() const -> const std::vector<std::shared_ptr<TokenRule>>&
    {
        return m_rules;
    }

    auto Token::to_string() const -> File::StringType
    {
        return m_debug_name;
    }

    auto Token::create(int type, File::StringViewType name, File::StringViewType identifier, HasData has_data) -> Token
    {
        Token token{type, name, identifier, has_data};
        return token;
    }
}
