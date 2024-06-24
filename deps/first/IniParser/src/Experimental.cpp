#include <IniParser/Experimental.hpp>

namespace RC::Parser::Experimental
{
    auto ExperimentalTokenParser::find_variable_by_name(Section* section, const StringType& name) -> std::optional<std::reference_wrapper<Value>>
    {
        auto const& var = section->key_value_pairs.find(name);
        if (var != section->key_value_pairs.end())
        {
            return std::ref(var->second);
        }
        else
        {
            return std::nullopt;
        }
    }

    auto ExperimentalTokenParser::find_variable_by_name(SectionContainer& sections, const StringType& name) -> std::optional<std::reference_wrapper<Value>>
    {
        std::optional<std::reference_wrapper<Value>> value_found = [&]() -> std::optional<std::reference_wrapper<Value>> {
            for (auto& [_, section] : sections)
            {
                const auto& maybe_value = find_variable_by_name(&section, name);
                if (maybe_value.has_value())
                {
                    return maybe_value;
                }
            }

            return std::nullopt;
        }();

        return value_found;
    }

    auto ExperimentalTokenParser::find_variable_by_name(const StringType& name) -> std::optional<std::reference_wrapper<Value>>
    {
        size_t occurrence_of_dot = name.find_first_of(STR('.'));
        if (occurrence_of_dot == name.npos || occurrence_of_dot + 1 > name.size())
        {
            return find_variable_by_name(m_current_section, name);
        }
        else
        {
            const auto& requested_section = m_output.find(name.substr(0, occurrence_of_dot));
            if (requested_section == m_output.end())
            {
                return find_variable_by_name(m_current_section, name);
            }
            else
            {
                const StringType requested_variable_name = name.substr(occurrence_of_dot + 1, name.size());
                return find_variable_by_name(&requested_section->second, requested_variable_name);
            }
        }
    }

    auto ExperimentalTokenParser::try_set_section_value(Value& pair_value, const Token& token, bool is_space_valid) -> void
    {
        const auto token_type = token.get_type();
        if (token_type == TokenType::EndOfFile || token_type == TokenType::NewLine)
        {
            pair_value.value = STR("");
            pair_value.ref = &pair_value;
        }
        else if (token_type == TokenType::Characters)
        {
            StringType value_data{};
            consume_continually([&](const Parser::Token& token) {
                const auto token_type = token.get_type();
                if (token_type == TokenType::EndOfFile || token_type == TokenType::NewLine)
                {
                    // Full value found
                    // Stop consuming
                    return true;
                }
                else if (token_type == TokenType::Characters)
                {
                    // Append data, it cannot referring to a variable
                    value_data.append(get_data(token));

                    // Consume another token
                    return false;
                }
                else if (token_type == TokenType::Space)
                {
                    // Append space
                    value_data.append(STR(" "));

                    // Consume another token
                    return false;
                }
                else
                {
                    // Consume another token
                    return false;
                }
            });

            const auto& maybe_other_variable = find_variable_by_name(value_data);
            if (maybe_other_variable.has_value())
            {
                pair_value.ref = maybe_other_variable->get().ref;
                m_variable_to_assign_to = &pair_value;
            }
            else
            {
                pair_value.value = value_data;
                pair_value.ref = &pair_value;
            }
        }
        else if (token_type == TokenType::Space && is_space_valid)
        {
            // Consume the space
            consume();

            // Consume all other contiguous spaces
            while (peek().get_type() == TokenType::Space)
            {
                consume();
            }

            const auto& next_token = peek();
            try_set_section_value(pair_value, next_token, false);
        }
        else
        {
            if (is_space_valid)
            {
                throw std::runtime_error{"Syntax error: Expected EndOfFile Space, or Characters"};
            }
            else
            {
                throw std::runtime_error{"Syntax error: Expected EndOfFile or Characters"};
            }
        }
    }

    auto ExperimentalTokenParser::handle_operator_equals(const Token& token) -> void
    {
        if (!m_current_section)
        {
            throw std::runtime_error{"Syntax error: No section. Global variables not supported, please create a [Section]."};
        }

        // Get the key of the key/value pair
        // Direction is: Backwards
        // Currently parsing: Equals
        // Peeking backwards would result in the token before Equals, which is either Characters or undefined number of spaces
        // If Characters, consume & resolve its data & create variable if it doesn't exist with the data as the name
        // If Space {
        //     Consume all spaces until a Characters or EndOfFile token is found
        //     If Characters, consume & resolve its data & create variable if it doesn't exist with the data as the name
        //     If EndOfFile, this is a syntax error, we've reached the start of the file without finding the key
        // }
        auto& section_value = [&]() -> Value& {
            const auto& previous_token = consume(PeekDirection::Backward);
            if (previous_token.get_type() == TokenType::EndOfFile)
            {
                throw std::runtime_error{"Syntax error: Expected Characters, got EndOfFile"};
            }
            else if (previous_token.get_type() == TokenType::Characters)
            {
                StringType key_name = get_data(previous_token);
                const auto& key_value_pair_iter = m_current_section->key_value_pairs.find(key_name);
                if (key_value_pair_iter == m_current_section->key_value_pairs.end())
                {
                    const auto& new_pair = m_current_section->key_value_pairs.emplace(key_name, Value{});
                    return new_pair.first->second;
                }
                else
                {
                    return key_value_pair_iter->second;
                }
            }
            else if (previous_token.get_type() == TokenType::Space)
            {
                // Consume all spaces
                while (peek(PeekDirection::Backward).get_type() == TokenType::Space)
                {
                    consume(PeekDirection::Backward);
                }

                const auto& characters_token = consume(PeekDirection::Backward);
                if (characters_token.get_type() == TokenType::EndOfFile)
                {
                    throw std::runtime_error{"Syntax error: Expected Characters, got EndOfFile"};
                }
                else if (characters_token.get_type() == TokenType::Characters)
                {
                    const StringType key_name = get_data(characters_token);
                    const auto& maybe_variable = find_variable_by_name(key_name);
                    if (maybe_variable.has_value())
                    {
                        return maybe_variable.value().get();
                    }
                    else
                    {
                        const auto& new_pair = m_current_section->key_value_pairs.emplace(key_name, Value{});
                        return new_pair.first->second;
                    }
                }
                else
                {
                    throw std::runtime_error{"Syntax error: Expected Characters"};
                }
            }
            else
            {
                throw std::runtime_error{"Syntax error: Expected Space or Characters"};
            }
        }();

        // Get the value of the key/value pair
        // Direction is: Forwards
        // Currently parsing: Equals
        // Peeking forwards would result in the token after Equals, which is either EndOfFile, NewLine, Characters, or undefined number of Space
        // If EndOfFile or NewLine, the value for the key/value pair is empty
        // If Characters {
        //     Create a temporary to store the pre-processed value
        //     Consume one token at a time, valid tokens are EndOfFile, NewLine, Characters and undefined number of Space
        //     If EndOfFile or NewLine, stop consuming
        //     If Characters, append its data to the temporary (it should not be resolved as it cannot refer to a variable)
        //     If Space, append a space to the temporary
        //     When the process of consuming tokens is done, resolve the temporary as it may be referring to a variable
        // }
        // If Space {
        //     Consume all contiguous Space tokens
        //     The next valid token is either EndOfFile or Characters
        //     If EndOfFile, the value for the key/value pair is empty
        //     If Characters, consume & resolve its data as it may be referring to a variable
        // }
        const auto& next_token = peek();
        try_set_section_value(section_value, next_token);
    }

    auto ExperimentalTokenParser::handle_operator_plus(const Token& token) -> void
    {
        // The "Plus" token is typically invoked after an "Equals" token, as such:
        // final_storage = lhs + rhs
        // There can also be additional "Plus" tokens on the rhs
        // Currently those are not dealt with

        if (!m_variable_to_assign_to)
        {
            throw std::runtime_error{"Syntax error: No variable to store the appended string in (expected: var = x + y + ...)"};
        }

        // Find lhs
        // May be a variable or a temporary (Characters token)
        bool lhs_is_variable{};
        bool lhs_is_temporary{};
        StringType lhs = [&]() {
            if (!m_temporary.empty())
            {
                lhs_is_temporary = true;
                return m_temporary;
            }
            else
            {
                const auto& lhs_token = peek_and_ignore_until(TokenType::Characters, PeekDirection::Backward);
                if (lhs_token.get_type() == TokenType::EndOfFile)
                {
                    throw std::runtime_error{"Syntax error: Expected 'Characters', got 'EndOfFile'"};
                }
                else
                {
                    const StringType lhs_data = get_data(lhs_token);
                    auto maybe_variable = find_variable_by_name(lhs_data);
                    if (maybe_variable.has_value())
                    {
                        lhs_is_variable = true;
                        return maybe_variable.value().get().ref->value;
                    }
                    else
                    {
                        return lhs_data;
                    }
                }
            }
        }();

        // Currently parsing: Plus
        // Peeking would result in the token after Plus, which is either Space or Characters
        // If Space, consume it

        // Consume rhs space
        {
            const auto& next_token = peek();
            if (next_token.get_type() == TokenType::Space)
            {
                consume();
            }
        }

        // Currently parsing: Space
        // Peeking would result in the token after Space, which is either Characters or undefined number of Space
        // This is the rhs value
        // If Characters, resolve its data as it may be refering to a variable
        // If Space {
        //     Consume as temporary
        //     Peek, valid tokens are: Space, Characters, Plus, NewLine
        //     If peek returns Space, consume as temporary
        //     If peek returns Characters, consume & treat as temporary, it may not refer to a variable
        //     If peek returns Plus or NewLine, that's the end of the rhs
        // }

        // Find rhs
        StringType rhs = [&]() {
            StringType rhs_temporary{};
            const auto& next_token = peek();

            if (next_token.get_type() == TokenType::EndOfFile)
            {
                throw std::runtime_error{"Syntax error: Expected Characters or Space(s), got EndOfFile"};
            }
            else if (next_token.get_type() == TokenType::Characters)
            {
                // Resolve Characters data, it may be refering to a variable
                // After that, consume
                auto maybe_variable = find_variable_by_name(get_data(next_token));
                if (maybe_variable.has_value())
                {
                    consume();
                    return maybe_variable.value().get().ref->value;
                }
                else
                {
                    consume();
                    return get_data(next_token);
                }
            }
            else if (next_token.get_type() == TokenType::Space)
            {
                rhs_temporary += STR(" ");
                consume(); // Consume the first Space

                consume_until(TokenType::Characters, [&](const Parser::Token& token) {
                    if (token.get_type() == TokenType::Space)
                    {
                        const auto& next_space_token = peek();
                        const auto token_type = next_space_token.get_type();
                        if (token_type == TokenType::EndOfFile || token_type == TokenType::Plus)
                        {
                            return true;
                        }
                        else
                        {
                            rhs_temporary += STR(" ");
                            return false;
                        }
                    }
                    else if (token.get_type() == TokenType::Characters)
                    {
                        rhs_temporary += get_data(token);
                        return false;
                    }
                    else
                    {
                        // NewLine, or tokens that are not supported for the Plus token
                        // This means there is nothing more to process for the rhs
                        return true;
                    }
                });

                return rhs_temporary;
            }
            else
            {
                throw std::runtime_error{"Syntax error: Expected Characters or Space(s)"};
            }
        }();

        m_temporary = lhs + rhs;

        // Now parsing: Characters
        // Peeking would result in the token after Characters, which is either Plus, NewLine, EndOfFile, or undefined number of Space
        // This is for determining whether this is the last Plus token to process before assigning to the variable
        // If Plus, do not assign to variable, simply return to let the parser continue
        // NewLine or EndOfFile, assign to variable and then return to let the parser continue
        // If Space {
        //     Consume & ignore
        //     Peek, valid tokens are: Space, Plus, NewLine or EndOfFile
        //     If peek returns Space, consume & ignore
        //     If peek returns Plus, do not assign to variable, simply return to let the parser continue
        //     If peek returns NewLine or EndOfFile, assign to variable and then return to let the parser continue
        // }

        bool is_last_operation = [&]() {
            const auto& next_token = peek();
            if (next_token.get_type() == TokenType::EndOfFile)
            {
                // EndOfFile
                return true;
            }
            else if (next_token.get_type() == TokenType::Space)
            {
                // Consume the first Space
                consume();

                bool is_last_operation_ret_value{};

                // Consume all subsequent Space tokens
                peek_until(TokenType::Characters, [&](const Parser::Token& token) {
                    if (token.get_type() == TokenType::Space)
                    {
                        return false;
                    }
                    else if (token.get_type() == TokenType::Plus)
                    {
                        is_last_operation_ret_value = false;
                        return true;
                    }
                    else if (token.get_type() == TokenType::NewLine)
                    {
                        is_last_operation_ret_value = true;
                        return true;
                    }
                    else
                    {
                        // Token is either EndOfFile or some other token that isn't supported here
                        is_last_operation_ret_value = true;
                        return true;
                    }
                });

                return is_last_operation_ret_value;
            }
            else if (next_token.get_type() == TokenType::Plus)
            {
                return false;
            }
            else if (next_token.get_type() == TokenType::NewLine)
            {
                return true;
            }
            else
            {
                // Token is either EndOfFile or some other token that isn't supported here
                return true;
            }
        }();

        if (is_last_operation)
        {
            m_variable_to_assign_to->value = m_temporary;
            m_variable_to_assign_to->ref = m_variable_to_assign_to;
            m_variable_to_assign_to = nullptr;
            m_temporary.clear();
        }
    }

    auto ExperimentalTokenParser::handle_opening_square_bracket(const Token& token) -> void
    {
        const auto& next_token_or_eof = consume();
        if (next_token_or_eof.get_type() != TokenType::EndOfFile)
        {
            const auto& section_name_token = [&]() {
                if (next_token_or_eof.get_type() == TokenType::Characters)
                {
                    // No preceding spaces
                    return next_token_or_eof;
                }
                else
                {
                    // There might be preceding spaces, consume them and check again
                    const auto& token = consume_and_ignore_until(TokenType::Characters);
                    if (token.get_type() == TokenType::EndOfFile)
                    {
                        throw std::runtime_error{"Syntax error: Expected 'Characters', got 'EndOfFile'"};
                    }
                    else
                    {
                        return token;
                    }
                }
            }();

            StringType section_name = get_data(section_name_token);
            if (auto section = m_output.find(section_name); section != m_output.end())
            {
                m_current_section = &section->second;
            }
            else
            {
                m_current_section = &m_output.emplace(section_name, Section{}).first->second;
            }
        }
    }

    auto ExperimentalTokenParser::handle_single_line_comment() -> void
    {
        consume_and_ignore_until(TokenType::NewLine);
    }

    auto ExperimentalTokenParser::parse_token(const Token& token) -> void
    {
        switch (token.get_type())
        {
        case TokenType::Equals:
            handle_operator_equals(token);
            break;
        case TokenType::Plus:
            handle_operator_plus(token);
            break;
        case TokenType::OpeningSquareBracket:
            handle_opening_square_bracket(token);
            break;
        case TokenType::SemiColon:
            handle_single_line_comment();
            break;
        default:
            // No other tokens need handling for this particular ini parser
            break;
        }
    }

    ExperimentalParser::ExperimentalParser(const StringType& input)
    {
        // Tokenize -> START
        Parser::Tokenizer tokenizer;
        tokenizer.set_available_tokens(std::move(create_available_tokens_for_tokenizer()));
        tokenizer.tokenize(input);
        // Tokenize -> END

        // Parse Tokens -> START
        ExperimentalTokenParser token_parser{tokenizer, input, m_sections};
        token_parser.parse();
        // Parse Tokens -> END
    }

    auto ExperimentalParser::create_available_tokens_for_tokenizer() -> Parser::TokenContainer
    {
        Parser::TokenContainer tc;

        tc.add(Parser::Token::create(TokenType::CarriageReturn, STR("CarriageReturn"), STR("\r")));
        tc.add(Parser::Token::create(TokenType::NewLine, STR("NewLine"), STR("\n")));
        tc.add(Parser::Token::create(TokenType::Space, STR("Space"), STR(" ")));
        tc.add(Parser::Token::create(TokenType::Characters,
                                     STR("Characters"),
                                     L"",
                                     Parser::Token::HasData::Yes)); // Empty identifier will match everything that no other token identifier matches
        tc.add(Parser::Token::create(TokenType::Equals, STR("Equals"), STR("=")));
        // tc.add(Parser::Token::create(IniFileToken::Plus, STR("Plus"), STR("+")));

        auto close_square_bracket_token = tc.add(Parser::Token::create(TokenType::ClosingSquareBracket, STR("CloseSquareBracket"), STR("]")));

        auto token =
                Parser::Token::create<TokenMustEndWithOppositeToken, TokenMustHaveCharsBeforeEnd>(TokenType::OpeningSquareBracket, STR("OpenSquareBracket"), STR("["));
        tc.add(std::move(token));

        tc.add(Parser::Token::create(TokenType::SemiColon, STR("SemiColon"), STR(";")));

        return tc;
    }

    auto ExperimentalParser::get_string(const StringType& section, const StringType& key, const StringType& default_value) -> StringType
    {
        auto section_iter = m_sections.find(section);
        if (section_iter == m_sections.end())
        {
            return default_value;
        }
        else
        {
            auto key_value_pairs = section_iter->second.key_value_pairs;
            auto key_value_pairs_iter = key_value_pairs.find(key);
            if (key_value_pairs_iter == key_value_pairs.end())
            {
                return default_value;
            }
            else
            {
                return key_value_pairs_iter->second.ref->value;
            }
        }
    }

    auto test() -> void
    {
        /*
[ SectionOne]
; This is a comment and as such should be ignored by the parser
var1 = 2
var2=var1
var3 = var2
var4 = SectionTwo.var1
var5 = 1 + var2 +     3 + 4
var6 = hello +  + world
var7 = 1 + 2 + 3 + 4
 */

        StringType config_str = LR"(
    [ SectionOne]
    ; This is a comment and as such should be ignored by the parser
    var1 = 2
    var2=var1
    var3 = var2
    var4 = SectionTwo.var1
    var5 = 1 + var2 +     3 + 4
    var6 = hello +  + world
    var7 = 1 + 2 + 3 + 4

    [SectionTwo]
    var1 =    5
    var2 = 4
    var3 = SectionOne.var3
    var4 =
    var5 = SectionOne.var2 + 2
    var6 =
    var7 = var1 + 1
    var8 = D:\SomePath\
    var9 = var8 + ContinuedPath\ToSomething
    var10 = 1 + 2 + var1 + 3 + var9 + 4

[AnotherSection]
var1 = 1
var2 = SectionTwo.var1
var3 = 3
var4=
var5 = a string with spaces
    )";

        printf_s("Creating experimental parser\n");
        ExperimentalParser parser{config_str};

        constexpr auto default_value[] = STR("<var_not_found>");
        printf_s("SectionOne.var1 = %S\n", parser.get_string(STR("SectionOne"), STR("var1"), default_value).c_str());
        printf_s("SectionOne.var2 = %S\n", parser.get_string(STR("SectionOne"), STR("var2"), default_value).c_str());
        printf_s("SectionOne.var3 = %S\n", parser.get_string(STR("SectionOne"), STR("var3"), default_value).c_str());
        printf_s("SectionOne.var4 = %S\n", parser.get_string(STR("SectionOne"), STR("var4"), default_value).c_str());
        printf_s("SectionOne.var5 = %S\n", parser.get_string(STR("SectionOne"), STR("var5"), default_value).c_str());
        printf_s("SectionOne.var6 = %S\n", parser.get_string(STR("SectionOne"), STR("var6"), default_value).c_str());
        printf_s("SectionOne.var7 = %S\n", parser.get_string(STR("SectionOne"), STR("var7"), default_value).c_str());
        printf_s("\n");
        printf_s("SectionTwo.var1 = %S\n", parser.get_string(STR("SectionTwo"), STR("var1"), default_value).c_str());
        printf_s("SectionTwo.var2 = %S\n", parser.get_string(STR("SectionTwo"), STR("var2"), default_value).c_str());
        printf_s("SectionTwo.var3 = %S\n", parser.get_string(STR("SectionTwo"), STR("var3"), default_value).c_str());
        printf_s("SectionTwo.var4 = %S\n", parser.get_string(STR("SectionTwo"), STR("var4"), default_value).c_str());
        printf_s("SectionTwo.var5 = %S\n", parser.get_string(STR("SectionTwo"), STR("var5"), default_value).c_str());
        printf_s("SectionTwo.var6 = %S\n", parser.get_string(STR("SectionTwo"), STR("var6"), default_value).c_str());
        printf_s("SectionTwo.var7 = %S\n", parser.get_string(STR("SectionTwo"), STR("var7"), default_value).c_str());
        printf_s("SectionTwo.var8 = %S\n", parser.get_string(STR("SectionTwo"), STR("var8"), default_value).c_str());
        printf_s("SectionTwo.var9 = %S\n", parser.get_string(STR("SectionTwo"), STR("var9"), default_value).c_str());
        printf_s("SectionTwo.var10 = %S\n", parser.get_string(STR("SectionTwo"), STR("var10"), default_value).c_str());
        printf_s("\n");
        printf_s("AnotherSection.var1 = %S\n", parser.get_string(STR("AnotherSection"), STR("var1"), default_value).c_str());
        printf_s("AnotherSection.var2 = %S\n", parser.get_string(STR("AnotherSection"), STR("var2"), default_value).c_str());
        printf_s("AnotherSection.var3 = %S\n", parser.get_string(STR("AnotherSection"), STR("var3"), default_value).c_str());
        printf_s("AnotherSection.var4 = %S\n", parser.get_string(STR("AnotherSection"), STR("var4"), default_value).c_str());
        printf_s("AnotherSection.var5 = %S\n", parser.get_string(STR("AnotherSection"), STR("var5"), default_value).c_str());
    }
} // namespace RC::Parser::Experimental
