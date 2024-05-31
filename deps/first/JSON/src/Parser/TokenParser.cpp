#include <algorithm>
#include <cwctype>
#include <format>
#include <stdexcept>
#include <string>

#include <Helpers/String.hpp>
#include <JSON/Parser/TokenParser.hpp>
#include <ParserBase/Token.hpp>

namespace RC::JSON::Parser
{
#define RC_ASSERT(Expression)                                                                                                                                  \
    if (!(Expression))                                                                                                                                         \
    {                                                                                                                                                          \
        throw std::runtime_error{"runtime assert: [" #Expression " == false]"};                                                                                \
    }

#define RC_VERIFY_SYNTAX(Expression)                                                                                                                           \
    if (!(Expression))                                                                                                                                         \
    {                                                                                                                                                          \
        const auto& err_token = get_token(m_current_token_index_being_parsed);                                                                                 \
        auto e = std::format("Syntax error! ({} : {}): Expected '{}' token, got '{}'.",                                                                        \
                             err_token.get_line() + 1,                                                                                                         \
                             err_token.get_column(),                                                                                                           \
                             token_type_to_string(m_expected_token),                                                                                           \
                             to_string(err_token.to_string()));                                                                                                \
        if (err_token.get_type() == TokenType::Characters)                                                                                                     \
        {                                                                                                                                                      \
            e.append(std::format("\nCharacters: '{}'", to_string(get_data(err_token))));                                                                         \
        }                                                                                                                                                      \
        throw std::runtime_error{e};                                                                                                                           \
    }

    static auto is_number(StringViewType data) -> bool
    {
        return std::ranges::all_of(data.begin(), data.end(), [&](const CharType c) {
            if constexpr (std::is_same_v<File::CharType, wchar_t>)
            {
                return std::iswdigit(c) != 0;
            }
            else
            {
                return std::isdigit(c) != 0;
            }
        });
    }

    auto ScopeStack::push(StoredType stored_type) -> StoredType
    {
        return m_storage.emplace_back(stored_type);
    }

    auto ScopeStack::pop() -> StoredType
    {
        RC_ASSERT(!m_storage.empty());

        // The bottom of the stack is presumed to be the global scope.
        // Never remove the global scope.
        if (m_storage.size() <= 1)
        {
            return m_storage.back();
        }
        auto& top_item = m_storage.back();
        m_storage.pop_back();
        return top_item;
    }

    auto ScopeStack::peek_top() -> StoredType&
    {
        return m_storage.back();
    }

    static auto has_only_spaces(const File::StringType& data) -> bool
    {
        return std::all_of(data.begin(), data.end(), [](File::CharType c) {
            return std::isspace(c) || c == '\n';
        });
    }

    auto TokenParser::do_comma_verification() -> void
    {
        bool require_comma{};

        if (m_scope_stack.peek_top().is<JSON::Object>() && check_and_peek_next_tokens(TokenType::ClosingCurlyBrace))
        {
            require_comma = true;
        }

        if (m_scope_stack.peek_top().is<JSON::Array>() && check_and_peek_next_tokens(TokenType::ClosingSquareBracket))
        {
            require_comma = true;
        }

        if (require_comma)
        {
            m_expected_token = TokenType::Comma;
            RC_VERIFY_SYNTAX(peek().get_type() == m_expected_token || !check_and_peek_next_tokens(TokenType::EndOfFile))
        }
    }

    auto TokenParser::check_and_peek_next_tokens(TokenType type_to_check_against) -> bool
    {
        int peek_counter{1};
        while (true)
        {
            int local_peek_counter = peek_counter;

            const auto& token = peek(local_peek_counter);
            if ((token.get_type() != TokenType::CarriageReturn && token.get_type() != TokenType::NewLine && token.get_type() != TokenType::Characters) ||
                (token.has_data() && !has_only_spaces(get_data(token))))
            {
                break;
            }
            ++peek_counter;
        }

        return peek(peek_counter).get_type() != type_to_check_against;
    }

    auto TokenParser::check_and_consume_next_tokens(const ParserBase::Token& token_in) -> bool
    {
        while ((peek().has_data() && has_only_spaces(get_data(peek()))) || peek().get_type() == TokenType::CarriageReturn || peek().get_type() == TokenType::NewLine)
        {
            consume();
        }

        if (peek().get_type() == m_expected_token)
        {
            return true;
        }
        else
        {
            consume();
            return false;
        }
    }

    auto TokenParser::handle_double_quote_token(const ParserBase::Token& token) -> void
    {
        m_string_started = !m_string_started;

        if (m_string_started)
        {
            m_expected_token = TokenType::Characters;
            m_defer_element_creation = true;
            RC_VERIFY_SYNTAX(peek().get_type() == TokenType::Characters)
        }
        else
        {
            m_defer_element_creation = false;
            if (m_current_state == State::ReadKey && !m_last_key.empty())
            {
                m_expected_token = TokenType::Colon;
                RC_VERIFY_SYNTAX(check_and_consume_next_tokens(token))
            }
            else
            {
                do_comma_verification();
            }
        }
    }

    auto TokenParser::handle_characters_token(const ParserBase::Token& token) -> void
    {
        if (m_current_state == State::ReadKey)
        {
            RC_VERIFY_SYNTAX(m_string_started)
            m_last_key.append(get_data(token));
        }
        else if (m_current_state == State::ReadValue)
        {
            auto data_raw = get_data(token);
            StringType data_no_spaces = data_raw;
            data_no_spaces.erase(std::remove_if(data_no_spaces.begin(),
                                                data_no_spaces.end(),
                                                [](wchar_t c) {
                                                    return std::isspace(c);
                                                }),
                                 data_no_spaces.end());

            auto is_valid_number = is_number(data_no_spaces);

            if (!m_string_started && is_valid_number)
            {
                do_comma_verification();

                m_last_value = std::make_unique<JSON::Number>(std::stoll(data_no_spaces, nullptr));
            }
            else if (!m_string_started)
            {
                // No string started (because no opening double quote).
                // It might be another token so ignore this Character token and continue with the next one.
                return;
            }
            else
            {
                m_expected_token = TokenType::DoubleQuote;
                RC_VERIFY_SYNTAX(m_string_started)
                if (!m_last_value)
                {
                    m_last_value = std::make_unique<JSON::String>();
                }
                m_last_value->as<JSON::String>()->get().append(std::move(data_raw));
            }
        }
    }

    auto TokenParser::handle_opening_curly_brace_token(const ParserBase::Token& token) -> void
    {
        if (m_current_state == State::ReadValue)
        {
            m_last_value = std::make_unique<JSON::Object>();
            auto object = m_scope_stack.add_element_to_top(std::move(m_last_key), std::move(m_last_value));
            m_scope_stack.push(static_cast<JSON::Object*>(object));
            m_last_key.clear();
            m_last_value = nullptr;
        }
        else
        {
            RC_ASSERT(!m_global_object)

            m_global_object = std::make_unique<JSON::Object>();
            m_scope_stack.push(m_global_object.get());
        }
        m_current_state = State::ReadKey;

        m_expected_token = TokenType::DoubleQuote;
        RC_VERIFY_SYNTAX(check_and_consume_next_tokens(peek()))
    }

    auto TokenParser::handle_closing_curly_brace_token(const ParserBase::Token& token) -> void
    {
        m_scope_stack.pop();
        do_comma_verification();
    }

    auto TokenParser::handle_opening_square_bracket_token(const ParserBase::Token& token) -> void
    {
        RC_VERIFY_SYNTAX(m_global_object)
        RC_VERIFY_SYNTAX(!m_scope_stack.empty())

        m_last_value = std::make_unique<JSON::Array>();
        Value* array{};
        if (m_scope_stack.peek_top().is<JSON::Object>())
        {
            array = m_scope_stack.add_element_to_top(std::move(m_last_key), std::move(m_last_value));
        }
        else
        {
            array = m_scope_stack.add_element_to_top(std::move(m_last_value));
        }
        m_scope_stack.push(static_cast<JSON::Array*>(array));
        m_last_value = nullptr;

        m_current_state = State::ReadValue;
    }

    auto TokenParser::handle_closing_square_bracket_token(const ParserBase::Token& token) -> void
    {
        m_scope_stack.pop();
    }

    auto TokenParser::handle_comma_token(const ParserBase::Token& token) -> void
    {
        const auto& top = m_scope_stack.peek_top();
        if (top.is<JSON::Object>())
        {
            m_current_state = State::ReadKey;
        }
        else if (top.is<JSON::Array>())
        {
            m_current_state = State::ReadValue;
        }
    }

    auto TokenParser::handle_colon_token(const ParserBase::Token& token) -> void
    {
        m_current_state = State::ReadValue;
    }

    auto TokenParser::handle_boolean_token(const ParserBase::Token& token, bool is_true) -> void
    {
        m_defer_element_creation = false;
        m_last_value = std::make_unique<JSON::Bool>(is_true);
    }

    auto TokenParser::parse_token(const ParserBase::Token& token) -> void
    {
        if (m_current_state == State::StartOfFile)
        {
            m_expected_token = TokenType::OpeningCurlyBrace;
            auto token_type = token.get_type();
            RC_VERIFY_SYNTAX(token_type == TokenType::OpeningCurlyBrace || (token_type == TokenType::Characters && has_only_spaces(get_data(token))));
            m_current_state = State::ProcessNext;
        }

        if (m_string_started && token.get_type() != TokenType::Characters && token.get_type() != TokenType::DoubleQuote)
        {
            if (m_current_state == State::ReadKey)
            {
                m_last_key.append(token.get_identifier());
            }
            else
            {
                if (!m_last_value)
                {
                    m_last_value = std::make_unique<JSON::String>();
                }
                m_last_value->as<JSON::String>()->get().append(token.get_identifier());
            }
            return;
        }

        if (!m_string_started && (token.has_data() && has_only_spaces(get_data(token))))
        {
            return;
        }

        switch (token.get_type())
        {
        case DoubleQuote:
            handle_double_quote_token(token);
            break;
        case Characters:
            handle_characters_token(token);
            break;
        case ClosingCurlyBrace:
            handle_closing_curly_brace_token(token);
            break;
        case OpeningCurlyBrace:
            handle_opening_curly_brace_token(token);
            break;
        case ClosingSquareBracket:
            handle_closing_square_bracket_token(token);
            break;
        case OpeningSquareBracket:
            handle_opening_square_bracket_token(token);
            break;
        case Comma:
            handle_comma_token(token);
            break;
        case Colon:
            handle_colon_token(token);
            break;
        case True:
            handle_boolean_token(token, true);
            break;
        case False:
            handle_boolean_token(token, false);
            break;
        case EndOfFile:
        default:
            // No other tokens need handling for this particular parser
            break;
        }

        if (!m_defer_element_creation)
        {
            const auto& top = m_scope_stack.peek_top();
            if (top.is<JSON::Object>() && !m_last_key.empty() && m_last_value)
            {
                m_scope_stack.add_element_to_top(std::move(m_last_key), std::move(m_last_value));
                m_last_key.clear();
                m_last_value = nullptr;
            }

            if (top.is<JSON::Array>() && m_last_value)
            {
                m_scope_stack.add_element_to_top(std::move(m_last_value));
                m_last_value = nullptr;
            }
        }
    }
} // namespace RC::JSON::Parser