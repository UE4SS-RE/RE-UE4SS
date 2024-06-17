#include <cwctype>
#include <format>

#include <Helpers/String.hpp>
#include <IniParser/TokenParser.hpp>
#include <IniParser/Tokens.hpp>
#include <IniParser/Value.hpp>
#include <fmt/core.h>
#include <ParserBase/Token.hpp>

namespace RC::Ini
{
    struct Int
    {
        int64_t value;
        int base{10};
        bool is_int{false};
    };

    struct Float
    {
        float value;
        bool is_float{false};
    };

    struct Bool
    {
        bool value;
        bool is_bool{false};
    };

    auto static is_int(File::StringViewType data) -> Int
    {
        bool has_0x_prefix = [&]() {
            return (data.size() > 2 && data[0] == L'0' && (data[1] == L'x' || data[1] == L'X'));
        }();

        if (!has_0x_prefix && data[0] != L'-' && std::iswdigit(data[0]) == 0)
        {
            return Int{0, 10, false};
        }
        else
        {
            File::StringViewType string = has_0x_prefix ? File::StringViewType{data.begin() + 2, data.end()} : data;
            if (!has_0x_prefix && data[0] == STR('-'))
            {
                string = File::StringViewType{string.begin() + 1, string.end()};
            }
            bool is_int = std::ranges::all_of(string.begin(), string.end(), [&](const File::CharType c) {
                if constexpr (std::is_same_v<File::CharType, wchar_t>)
                {
                    return has_0x_prefix ? std::iswxdigit(c) : std::iswdigit(c) != 0;
                }
                else
                {
                    return has_0x_prefix ? std::isxdigit(c) : std::isdigit(c) != 0;
                }
            });

            return Int{.value = 0, .base = has_0x_prefix ? 16 : 10, .is_int = is_int};
        }
    }

    auto static is_float(File::StringViewType data) -> Float
    {
        bool has_decimal_or_negative_prefix = [&]() {
            return data.size() > 1 && data[0] == L'.' || data[0] == L'-';
        }();

        if (!has_decimal_or_negative_prefix && std::iswdigit(data[0]) == 0)
        {
            return Float{0, false};
        }
        else
        {
            File::StringViewType string = data.ends_with(STR('f')) ? File::StringViewType{data.begin(), data.end() - 1} : data;
            if (has_decimal_or_negative_prefix)
            {
                string = File::StringViewType{string.begin() + 1, string.end()};
            }
            bool is_float = std::ranges::all_of(string.begin(), string.end(), [&](const File::CharType c) {
                if constexpr (std::is_same_v<File::CharType, wchar_t>)
                {
                    return has_decimal_or_negative_prefix ? std::iswxdigit(c) : std::iswdigit(c) != 0 || c == STR('.');
                }
                else
                {
                    return has_decimal_or_negative_prefix ? std::isxdigit(c) : std::isdigit(c) != 0 || c == STR('.');
                }
            });

            return Float{.value = 0, .is_float = is_float};
        }
    }

    auto static is_bool(const File::StringType& data) -> Bool
    {
        File::StringType all_lower_string_data = data;
        // TODO: This to_lower implementation is not string-type agnostic
        //       A code change would be required if 'File::StringType' is defined as a char instead of a wchar_t
        //       Solution: Make two overloads in the string helper library, one for 'std::string' and one for 'std::wstring'
        std::transform(all_lower_string_data.begin(), all_lower_string_data.end(), all_lower_string_data.begin(), [](wchar_t c) {
            return std::towlower(c);
        });
        if (all_lower_string_data == STR("true") || all_lower_string_data == STR("1"))
        {
            return Bool{.value = true, .is_bool = true};
        }
        else if (all_lower_string_data == STR("false") || all_lower_string_data == STR("0"))
        {
            return Bool{.value = false, .is_bool = true};
        }
        else
        {
            return Bool{.value = false, .is_bool = false};
        }
    }

    auto TokenParser::find_variable_by_name(Section* section, const RC::StringType& name) -> std::optional<std::reference_wrapper<Value>>
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

    auto TokenParser::find_variable_by_name(const StringType& name) -> std::optional<std::reference_wrapper<Value>>
    {
        size_t occurrence_of_dot = name.find_first_of(L'.');
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
                const File::StringType requested_variable_name = name.substr(occurrence_of_dot + 1, name.size());
                return find_variable_by_name(&requested_section->second, requested_variable_name);
            }
        }
    }

    auto state_to_string(State state) -> File::StringType
    {
        switch (state)
        {
        case State::StartOfFile:
            return STR("StartOfFile");
        case State::SetSectionValue:
            return STR("SetSectionValue");
        case State::NewLineStarted:
            return STR("NewLineStarted");
        case State::CreateNewOrSetCurrentSection:
            return STR("CreateNewOrSetCurrentSection");
        case State::CreateSectionKey:
            return STR("CreateSectionKey");
        case State::StoreSectionKey:
            return STR("StoreSectionKey");
        }

        return STR("UnknownState");
    }

    auto TokenParser::characters_to_string(const ParserBase::Token& characters_token) -> File::StringType
    {
        auto* current_token = &characters_token;

        File::StringType full_value{};
        while (true)
        {
            const auto token_type = current_token->get_type();
            if (token_type == IniTokenType::EndOfFile || token_type == IniTokenType::CarriageReturn)
            {
                break;
            }
            else if (token_type == IniTokenType::Space)
            {
                full_value.append(STR(" "));
            }
            else if (token_type == IniTokenType::Characters)
            {
                full_value.append(get_data(*current_token));
            }
            else if (token_type == IniTokenType::Equals)
            {
                full_value.append(STR("="));
            }
            else if (token_type == IniTokenType::ClosingSquareBracket)
            {
                const auto next_token_type = peek().get_type();
                if (next_token_type == IniTokenType::CarriageReturn || next_token_type == IniTokenType::NewLine)
                {
                    // We're creating a string for the section name
                    // The next token is the last token on this line and since it's ] we don't want to include it in the string
                    break;
                }
                full_value.append(STR("]"));
            }
            else if (token_type == IniTokenType::OpeningSquareBracket)
            {
                full_value.append(STR("["));
            }
            else if (token_type == IniTokenType::SemiColon)
            {
                full_value.append(STR(";"));
            }

            // Exit early and let the state machine deal with the new line
            const auto next_token_type = peek().get_type();
            if (next_token_type == IniTokenType::CarriageReturn || next_token_type == IniTokenType::NewLine)
            {
                break;
            }

            // Prevent the last ] from being consumed, it has to be processed later
            if (m_current_state == State::CreateNewOrSetCurrentSection)
            {
                if (peek().get_type() == IniTokenType::ClosingSquareBracket)
                {
                    const auto two_over_token_type = peek(2).get_type();
                    if (two_over_token_type == IniTokenType::CarriageReturn || two_over_token_type == IniTokenType::NewLine)
                    {
                        break;
                    }
                }
            }

            current_token = &consume();
        }

        return full_value;
    }

    auto TokenParser::handle_opening_square_bracket_token(const ParserBase::Token& token) -> void
    {
        // Handling a special case for our unquoted strings
        if (m_current_state == State::SetSectionValue)
        {
            handle_characters_token(token);
            return;
        }

        if (m_current_state != State::NewLineStarted && m_current_state != State::StartOfFile)
        {
            throw std::runtime_error{fmt::format("Syntax error({} : {}): Expected state NewLineStarted or StartOfFile, got {}",
                                                 token.get_line(),
                                                 token.get_column(),
                                                 to_string(state_to_string(m_current_state)))};
        }

        m_current_state = State::CreateNewOrSetCurrentSection;
    }

    auto TokenParser::handle_closing_square_bracket_token(const ParserBase::Token& token) -> void
    {
        // Handling a special case for our unquoted strings
        if (m_current_state == State::SetSectionValue)
        {
            handle_characters_token(token);
            return;
        }

        if (m_current_character_data.empty())
        {
            throw std::runtime_error{
                    fmt::format("Syntax error ({} : {}): Expected Characters, got {}", token.get_line(), token.get_column(), to_string(token.to_string()))};
        }

        if (auto section = m_output.find(m_current_character_data); section != m_output.end())
        {
            m_current_section = &section->second;
        }
        else
        {
            m_current_section = &m_output.emplace(m_current_character_data, Section{}).first->second;
        }

        m_current_character_data.clear();
        m_current_state = State::StoreSectionKey;
    }

    auto TokenParser::handle_space_token([[maybe_unused]] const ParserBase::Token& token) -> void
    {
        if (m_current_state == State::CreateNewOrSetCurrentSection)
        {
            m_current_character_data.append(STR(" "));
        }
    }

    auto TokenParser::handle_characters_token(const ParserBase::Token& token) -> void
    {
        if (m_current_state == State::CreateNewOrSetCurrentSection)
        {
            m_current_character_data.append(characters_to_string(token));
        }
        else if (m_current_state == State::StoreSectionKey || m_current_state == State::NewLineStarted)
        {
            m_current_character_data = get_data(token);
            m_current_state = State::CreateSectionKey;
        }
        else if (m_current_state == State::SetSectionValue)
        {
            const auto value = characters_to_string(token);
            const auto maybe_other_variable = find_variable_by_name(value);
            if (maybe_other_variable.has_value())
            {
                m_current_value->set_ref(maybe_other_variable->get().get_ref());
            }
            else
            {
                m_current_value->add_string_value(value);

                if (auto int_data = is_int(value); int_data.is_int)
                {
                    m_current_value->add_int64_value(value, int_data.base);
                }

                if (auto float_data = is_float(value); float_data.is_float)
                {
                    m_current_value->add_float_value(value);
                }

                if (auto bool_data = is_bool(value); bool_data.is_bool)
                {
                    m_current_value->add_bool_value(bool_data.value);
                }
            }
        }
        else
        {
            throw std::runtime_error{
                    fmt::format("Syntax error({} : {}): Invalid state {}", token.get_line(), token.get_column(), to_string(state_to_string(m_current_state)))};
        }
    }

    auto TokenParser::handle_equals_token(const ParserBase::Token& token) -> void
    {
        // Handling a special case for our unquoted strings
        if (m_current_state == State::SetSectionValue)
        {
            handle_characters_token(token);
            return;
        }

        if (m_current_state != State::CreateSectionKey)
        {
            throw std::runtime_error{fmt::format("Syntax error({} : {}): Expected state CreateSectionKey, got {}",
                                                 token.get_line(),
                                                 token.get_column(),
                                                 to_string(state_to_string(m_current_state)))};
        }

        if (!m_current_section)
        {
            throw std::runtime_error{fmt::format("Syntax error ({} : {}): No section. Global variables not supported, please create a [Section]",
                                                 token.get_line(),
                                                 token.get_column())};
        }

        if (m_current_character_data.empty())
        {
            throw std::runtime_error{
                    fmt::format("Syntax error ({} : {}): Expected Characters, got {}", token.get_line(), token.get_column(), to_string(token.to_string()))};
        }

        if (m_current_section->is_ordered_list)
        {
            throw std::runtime_error{
                    fmt::format("Syntax error ({} : {}): Previous item is in ordered-list mode, expected another list item, got key/value pair",
                                token.get_line(),
                                token.get_column(),
                                to_string(token.to_string()))};
        }

        // Create the value with the correct key and an empty value and store a pointer to it so that the value can be set later
        m_current_value = &m_current_section->key_value_pairs.emplace(m_current_character_data, Value{}).first->second;
        m_current_value->add_string_value(STR(""));
        m_current_value->set_ref(m_current_value);
        m_current_character_data.clear();

        m_current_state = State::SetSectionValue;
    }

    auto TokenParser::handle_new_line_token([[maybe_unused]] const ParserBase::Token& token) -> void
    {
        if (m_current_section && !m_current_character_data.empty())
        {
            // If the Equals token hasn't set 'is_ordered_list' then we set it here if there aren't any items in the section yet.
            if (m_current_section->key_value_pairs.empty() && !m_current_section->is_ordered_list)
            {
                m_current_section->is_ordered_list = true;
            }

            // If the Equals token has set 'is_ordered_list', and the section already has values, then we have previous items that aren't in ordered-list mode
            if (!m_current_section->key_value_pairs.empty() && !m_current_section->is_ordered_list)
            {
                throw std::runtime_error{
                        fmt::format("Syntax error ({} : {}): Previous item is in key/value mode, expected another key/value item, got ordered-list item",
                                    token.get_line(),
                                    token.get_column(),
                                    to_string(token.to_string()))};
            }

            if (m_current_section->is_ordered_list)
            {
                m_current_section->ordered_list.emplace_back(m_current_character_data);
            }
        }

        m_current_character_data.clear();
        m_current_value = nullptr;

        if (m_current_state != State::StoreSectionKey)
        {
            m_current_state = State::NewLineStarted;
        }
    }

    auto TokenParser::handle_semi_colon_token([[maybe_unused]] const ParserBase::Token& token) -> void
    {
        // Handling a special case for our unquoted strings
        if (m_current_state == State::SetSectionValue)
        {
            handle_characters_token(token);
            return;
        }

        while (peek().get_type() != IniTokenType::NewLine)
        {
            consume();
        }
    }

    auto TokenParser::parse_token(const ParserBase::Token& token) -> void
    {
        switch (token.get_type())
        {
        case CarriageReturn:
            break;
        case NewLine:
            handle_new_line_token(token);
            break;
        case Space:
            handle_space_token(token);
            break;
        case Characters:
            handle_characters_token(token);
            break;
        case Equals:
            handle_equals_token(token);
            break;
        case ClosingSquareBracket:
            handle_closing_square_bracket_token(token);
            break;
        case OpeningSquareBracket:
            handle_opening_square_bracket_token(token);
            break;
        case SemiColon:
            handle_semi_colon_token(token);
            break;
        case EndOfFile:
        default:
            // No other tokens need handling for this particular ini parser
            break;
        }
    }
} // namespace RC::Ini
