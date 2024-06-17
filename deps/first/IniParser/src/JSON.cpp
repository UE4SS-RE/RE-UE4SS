#include <format>

#include <IniParser/JSON.hpp>
#include <ParserBase/Token.hpp>
#include <ParserBase/Tokenizer.hpp>

namespace RC::Parser
{
    static auto to_string(std::wstring in) -> std::string
    {
        return std::string{in.begin(), in.end()};
    }

    static auto has_only_spaces(const File::StringType& data) -> bool
    {
        if (std::all_of(data.begin(), data.end(), [](File::CharType c) {
                return std::isspace(c) || c == '\n';
            }))
        {
            printf_s("SKIPPING\n");
            return true;
        }
        else
        {
            printf_s("NOT SKIPPING, data: '%S'\n", data.c_str());
            return false;
        }
    }

    auto JSONInternal::ItemBase::get_name() -> File::StringViewType
    {
        if (m_is_global_scope)
        {
            return STR("-- IS ANONYMOUS GLOBAL --");
        }
        else
        {
            return m_name;
        }
    }

    auto JSONInternal::StringItem::to_string() -> File::StringType
    {
        return fmt::format(STR("String = \"{}\""), m_value);
    }

    auto JSONInternal::ObjectScope::to_string() -> File::StringType
    {
        File::StringType str = fmt::format(STR("Object = \"{}\""), get_name());

        for (const auto& member : m_members)
        {
            str.append(fmt::format(STR("\n\"{}\" = {}"), member->get_name(), member->to_string()));
        }

        return str;
    }

    auto JSONInternal::ArrayScope::to_string() -> File::StringType
    {
        return fmt::format(STR("Array = \"{}\""), get_name());
    }

    auto JSONInternal::TokenParser::token_to_string(const Token& token) -> File::StringType
    {
        File::StringType string{};

        // TODO: Support manual escaping of double quotes with '\'
        while (peek().get_type() != TokenType::DoubleQuote)
        {
            // Get the next token after the 'DoubleQuote' token
            auto& next_token = consume();

            // Auto-escaping all token identifiers after the opening double quote
            if (next_token.get_type() == TokenType::Characters)
            {
                string.append(get_data(next_token));
            }
            else
            {
                string.append(next_token.get_identifier());
            }
        }

        return string;
    }

    auto JSONInternal::TokenParser::skip_all_spaces() -> void
    {
        auto& peeked_token = peek();
        if (peeked_token.get_type() == TokenType::Characters)
        {
            // We have either a non-string value or we have spaces which need to be ignored
            if (has_only_spaces(get_data(peeked_token)))
            {
                consume();
                printf_s("Consumed '%S', which contained: '%S'\n", peeked_token.to_string().c_str(), get_data(peeked_token).c_str());
            }
        }
    }

    auto JSONInternal::TokenParser::parse_open_curly_brace(const Token& token) -> void
    {
        bool is_first_item = !m_current_item;

        // Open array scope
        m_current_item = m_items.emplace_back(std::make_unique<ObjectScope>()).get();

        if (is_first_item)
        {
            m_current_item->m_is_global_scope = true;
        }

        // Right after opening an object, the only valid token is a quoted string
        m_next_token_expected = TokenType::DoubleQuote;
        m_processed_token_types += TokenType::OpenCurlyBrace;
    }

    auto JSONInternal::TokenParser::parse_close_curly_brace(const Token& token) -> void
    {
        if (m_current_item->get_type() == ItemType::Object)
        {
            if (static_cast<ObjectScope*>(m_current_item)->m_previous_line_without_comma == 0)
            {
                throw std::runtime_error{
                        fmt::format("Syntax error ({} : {}): Unexpected 'Comma' token, expected 'CloseCurlyBrace'.", token.get_line(), token.get_column())};
            }
        }
        else if (m_current_item->get_type() == ItemType::Array)
        {
        }

        m_processed_token_types += TokenType::CloseCurlyBrace;
    }

    auto JSONInternal::TokenParser::parse_colon(const Token& token) -> void
    {
        // This might be a complicated token
        // We don't know the type of item that we're creating yet, so we can't just set the token type

        skip_all_spaces();

        auto token_type = peek().get_type();
        if (token_type == TokenType::DoubleQuote)
        {
            // Consume the opening 'DoubleQuote' token
            parse_double_quote(consume());

            auto value = token_to_string(peek());

            if (m_current_item->get_type() == ItemType::Object)
            {
                auto& typed_current_item = static_cast<ObjectScope&>(*m_current_item);

                // Enforce 'Comma' token, if this isn't the first item
                if (!typed_current_item.m_members.empty() && typed_current_item.m_previous_line_without_comma != 0)
                {
                    throw std::runtime_error{fmt::format("Syntax error ({} : {}): Expected 'Comma' token before new item.",
                                                         typed_current_item.m_previous_line_without_comma,
                                                         typed_current_item.m_previous_column_without_comma)};
                }

                auto& item = typed_current_item.m_members.emplace_back(std::make_unique<StringItem>(value));
                printf_s("Adding string, '%S', to object, it has the name '%S'\n", value.c_str(), m_string_value_buffer.c_str());
                item->m_name = std::move(m_string_value_buffer);
                m_string_value_buffer.clear();

                auto& next_token = peek();
                typed_current_item.m_previous_line_without_comma = next_token.get_line();
                typed_current_item.m_previous_column_without_comma = next_token.get_column();
            }
            else if (m_current_item->get_type() == ItemType::Array)
            {
                throw std::runtime_error{"Invalid scope: Arrays are not yet implemented"};
            }
            else
            {
                throw std::runtime_error{"Invalid scope: No type"};
            }

            // Consume the closing 'DoubleQuote' token
            if (peek().get_type() != TokenType::DoubleQuote)
            {
                throw std::runtime_error{fmt::format("Syntax error ({} : {}): Expected 'DoubleQuote' token, got '{}'.",
                                                     token.get_line(),
                                                     token.get_column(),
                                                     to_string(token.to_string()))};
            }
            parse_double_quote(consume());

            m_next_token_expected = TokenType::Default;
        }
        else if (token_type == TokenType::OpenCurlyBrace)
        {
            // Consider returning here without doing anything
            // That would let the main handler deal with this
            // You would need some way of telling the handler that the thing (object in this case) being created is part of
            // an object or array, and also which object or array that it's part of
            throw std::runtime_error{"Object in object or object in array\n"};
        }
        else if (token_type == TokenType::OpenSquareBracket)
        {
            // Consider returning here without doing anything
            // That would let the main handler deal with this
            // You would need some way of telling the handler that the thing (object in this case) being created is part of
            // an object or array, and also which object or array that it's part of
            throw std::runtime_error{"Array in object or array in array\n"};
        }
        else
        {
            // No valid for now
            throw std::runtime_error{"Unsupported type after 'Colon' token, expected 'DoubleQuote', 'OpenCurlyBrace', or 'OpenSquareBracket'"};
        }

        m_processed_token_types += TokenType::Colon;
    }

    auto JSONInternal::TokenParser::parse_comma(const Token& token) -> void
    {
        auto item_type = m_current_item->get_type();
        if (item_type == ItemType::Object)
        {
            ObjectScope& current_item = static_cast<ObjectScope&>(*m_current_item);
            current_item.m_previous_line_without_comma = 0;
            current_item.m_previous_column_without_comma = 0;
        }
        else if (item_type == ItemType::Array)
        {
        }
        else
        {
            throw std::runtime_error{fmt::format("Syntax error ({} : {}): Unexpected 'Comma' token.", token.get_line(), token.get_column())};
        }

        m_processed_token_types += TokenType::Comma;
    }

    auto JSONInternal::TokenParser::parse_double_quote(const Token& token) -> void
    {
        if (!m_double_quote_opened)
        {
            m_next_token_expected = TokenType::Characters;
            m_double_quote_opened = true;
        }
        else
        {
            // If last processed token was a 'Comma', 'OpenCurlyBrace', or 'OpenSquareBracket', then a colon is expected
            // This means that this is the LHS of an assignment, not the RHS
            // Therefore we expect the next token to be an assignment token (Colon)
            auto last_processed_token_type = m_processed_token_types[-1];
            if (last_processed_token_type == TokenType::Comma || last_processed_token_type == TokenType::OpenCurlyBrace ||
                last_processed_token_type == TokenType::OpenSquareBracket)
            {
                skip_all_spaces();

                auto& peeked_token = peek();
                if (peeked_token.get_type() != TokenType::Colon)
                {
                    throw std::runtime_error{fmt::format("Syntax error ({} : {}): Expected 'Colon' token, got '{}'.",
                                                         peeked_token.get_line(),
                                                         peeked_token.get_column(),
                                                         to_string(peeked_token.to_string()))};
                }
            }

            m_double_quote_opened = false;
            m_double_quote_successfully_closed = true;
            m_next_token_expected = TokenType::Default;
        }

        m_processed_token_types += TokenType::DoubleQuote;
    }

    auto JSONInternal::TokenParser::parse_characters(const Token& token) -> void
    {
        auto& data = get_data(token);

        // Ignore if the data only contains spaces
        if (has_only_spaces(data))
        {
            return;
        }

        m_string_value_buffer = std::move(data);

        // Simulate that we dealt with the token successfully until there's a proper implementation
        m_next_token_expected = TokenType::Default;
    }

    auto JSONInternal::TokenParser::release_contents() -> std::vector<std::unique_ptr<ItemBase>>
    {
        return std::move(m_items);
    }

    auto JSONInternal::TokenParser::parse_token(const Token& token) -> void
    {
        TokenType token_type = static_cast<TokenType>(token.get_type());

        printf_s("current: %S <-> previous: %S <-> next: %S <-> expected: %S\n",
                 token.to_string().data(),
                 peek(PeekDirection::Backward).to_string().data(),
                 peek(PeekDirection::Forward).to_string().data(),
                 token_type_to_string(m_next_token_expected).data());

        // If type is 'Default', then this is the first token
        // If type is 'Characters', then let the 'Characters' handler handle this as a 'Characters' token may be ignored if it's only spaces
        // Otherwise, if the type is not what is expected, throw an exception
        if (m_next_token_expected != TokenType::Default && token_type != TokenType::Characters && token_type != m_next_token_expected)
        {
            throw std::runtime_error{
                    fmt::format("Syntax error ({} : {}): Expected 'Colon' token, got '{}'.", token.get_line(), token.get_column(), to_string(token.to_string()))};
        }

        switch (token_type)
        {
        case Default:
            break;
        case OpenCurlyBrace:
            parse_open_curly_brace(token);
            break;
        case CloseCurlyBrace:
            parse_close_curly_brace(token);
            break;
        case OpenSquareBracket:
            break;
        case CloseSquareBracket:
            break;
        case Colon:
            parse_colon(token);
            break;
        case Comma:
            parse_comma(token);
            break;
        case DoubleQuote:
            parse_double_quote(token);
            break;
        case Characters:
            parse_characters(token);
            break;
        case EndOfFile:
            break;
        }
    }

    auto JSON::parse_internal(File::StringType& input) -> void
    {
        Tokenizer tokenizer;
        TokenContainer tc;
        tc.add(Token::create(JSONInternal::TokenType::OpenCurlyBrace, STR("OpenCurlyBrace"), STR("{")));
        tc.add(Token::create(JSONInternal::TokenType::CloseCurlyBrace, STR("CloseCurlyBrace"), STR("}")));
        tc.add(Token::create(JSONInternal::TokenType::OpenSquareBracket, STR("OpenSquareBracket"), STR("[")));
        tc.add(Token::create(JSONInternal::TokenType::CloseSquareBracket, STR("CloseSquareBracket"), STR("]")));
        tc.add(Token::create(JSONInternal::TokenType::Colon, STR("Colon"), STR(":")));
        tc.add(Token::create(JSONInternal::TokenType::Comma, STR("Comma"), STR(",")));
        tc.add(Token::create(JSONInternal::TokenType::DoubleQuote, STR("DoubleQuote"), STR("\"")));
        tc.add(Token::create(JSONInternal::TokenType::Characters, STR("Characters"), STR(""), Token::HasData::Yes));
        tc.set_eof_token(JSONInternal::TokenType::EndOfFile);
        tokenizer.set_available_tokens(std::move(tc));
        tokenizer.tokenize(input);

        m_token_parser = std::make_unique<JSONInternal::TokenParser>(tokenizer, input);
        m_token_parser->parse();
    }

    auto JSON::parse(File::StringType& input) -> void
    {
        parse_internal(input);
    }

    auto JSON::parse(File::Handle& file) -> void
    {
        auto input = file.read_all();
        parse_internal(input);
    }

    auto JSON::release_contents() -> std::vector<std::unique_ptr<JSONInternal::ItemBase>>
    {
        return m_token_parser->release_contents();
    }

    auto JSON::test() -> void
    {
        // Current problems:
        /*
            No escaping in strings
            All token types would need to be escaped, and I hate this, things like commas should be auto-escaped in strings
            In fact, everything that's not a double quote should be auto-escaped in strings

            Development is very early on, this is an experiment but hopefully eventually it will be good enough to use
            I suspect that the recursive-ness of JSON is going to be a problem
        */

        /**/
        File::StringType input = LR"(
{
    "my key": "my string, value",
    "my second key": "my other string value",
    "my third key": {
        "my fourth key: "some other string value"
    }
})"; //*/

        JSON parser;
        parser.parse(input);

        // The content gets moved, the owner of the vector is now the current scope.
        auto items = parser.release_contents();
        for (const auto& item : items)
        {
            printf_s("%S\n", item->to_string().c_str());
        }
    }
} // namespace RC::Parser
