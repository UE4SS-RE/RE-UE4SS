#include <IniParser/Ini.hpp>
#include <IniParser/TokenParser.hpp>
#include <IniParser/Tokens.hpp>
#include <Helpers/String.hpp>

namespace RC::Ini
{
    auto Parser::parse_internal(SystemStringType& input) -> void
    {
        // Tokenize -> START
        ParserBase::Tokenizer tokenizer;
        tokenizer.set_available_tokens(create_available_tokens_for_tokenizer());
        tokenizer.tokenize(input);
        // Tokenize -> END

        // Parse Tokens -> START
        TokenParser token_parser{tokenizer, input, m_sections};
        token_parser.parse();
        // Parse Tokens -> END

        m_parsing_is_complete = true;
    }

    auto Parser::create_available_tokens_for_tokenizer() -> ParserBase::TokenContainer
    {
        ParserBase::TokenContainer tc;

        tc.add(ParserBase::Token::create(IniTokenType::CarriageReturn, SYSSTR("CarriageReturn"), SYSSTR("\r")));
        tc.add(ParserBase::Token::create(IniTokenType::NewLine, SYSSTR("NewLine"), SYSSTR("\n")));
        tc.add(ParserBase::Token::create(IniTokenType::Space, SYSSTR("Space"), SYSSTR(" ")));
        tc.add(ParserBase::Token::create(IniTokenType::Characters,
                                         SYSSTR("Characters"),
                                         SYSSTR(""),
                                         ParserBase::Token::HasData::Yes)); // Empty identifier will match everything that no other token identifier matches
        tc.add(ParserBase::Token::create /*<IniInternal::TokenEqualsAlwaysHaveCharactersUntilEndLine>*/ (IniTokenType::Equals, SYSSTR("Equals"), SYSSTR("=")));
        tc.add(ParserBase::Token::create(IniTokenType::ClosingSquareBracket, SYSSTR("CloseSquareBracket"), SYSSTR("]")));
        tc.add(ParserBase::Token::create(IniTokenType::OpeningSquareBracket, SYSSTR("OpenSquareBracket"), SYSSTR("[")));
        tc.add(ParserBase::Token::create(IniTokenType::SemiColon, SYSSTR("SemiColon"), SYSSTR(";")));

        tc.set_eof_token(IniTokenType::EndOfFile);

        return tc;
    }

    auto Parser::get_value(const SystemStringType& section, const SystemStringType& key, CanThrow can_throw) const
            -> std::optional<std::reference_wrapper<const Value>>
    {
        if (!m_parsing_is_complete)
        {
            if (can_throw == CanThrow::Yes)
            {
                throw std::runtime_error{"Call to Ini::get_value before parsing completed"};
            }
            else
            {
                return std::nullopt;
            }
        }

        const auto& section_iter = m_sections.find(section);
        if (section_iter == m_sections.end())
        {
            return std::nullopt;
        }
        else
        {
            const auto& key_value_pairs = section_iter->second.key_value_pairs;
            const auto& key_value_pairs_iter = key_value_pairs.find(key);
            if (key_value_pairs_iter == key_value_pairs.end())
            {
                return std::nullopt;
            }
            else
            {
                return std::ref(key_value_pairs_iter->second);
            }
        }
    }

    auto Parser::parse(SystemStringType& input) -> void
    {
        parse_internal(input);
    }

    auto Parser::parse(const File::Handle& file) -> void
    {
        auto input = file.read_file_all();
        parse_internal(input);
    }

    auto Parser::get_list(const SystemStringType& section) -> List
    {

        const auto& section_iter = m_sections.find(section);
        if (section_iter == m_sections.end())
        {
            return List{nullptr};
        }
        else
        {
            return List{&section_iter->second};
        }
    }

    auto Parser::get_ordered_list(const SystemStringType& section) -> List
    {
        return get_list(section);
    }

    auto Parser::get_string(const SystemStringType& section, const SystemStringType& key, const SystemStringType& default_value) const noexcept
            -> const SystemStringType&
    {
        const auto maybe_value = get_value(section, key, CanThrow::No);
        if (!maybe_value.has_value())
        {
            return default_value;
        }
        else
        {
            const auto& value = maybe_value.value().get();
            if (!value.get_ref()->is_valid_string())
            {
                return default_value;
            }
            else
            {
                return value.get_ref()->get_string_value();
            }
        }
    }

    auto Parser::get_string(const SystemStringType& section, const SystemStringType& key) const -> const SystemStringType&
    {
        const auto maybe_value = get_value(section, key);
        if (!maybe_value.has_value())
        {
            throw std::runtime_error{"[Ini::get_string] Tried getting value of type 'String' but the value didn't exist."};
        }
        else
        {
            const auto& value = maybe_value.value().get();
            if (!value.get_ref()->is_valid_string())
            {
                throw std::runtime_error{"[Ini::get_string] Tried getting value of type 'String' but the variable cannot be interpreted as 'String'"};
            }
            else
            {
                return value.get_ref()->get_string_value();
            }
        }
    }

    auto Parser::get_int64(const SystemStringType& section, const SystemStringType& key, int64_t default_value) const noexcept -> int64_t
    {
        const auto maybe_value = get_value(section, key, CanThrow::No);
        if (!maybe_value.has_value())
        {
            return default_value;
        }
        else
        {
            const auto& value = maybe_value.value().get();
            if (!value.get_ref()->is_valid_int64())
            {
                return default_value;
            }
            else
            {
                return value.get_ref()->get_int64_value();
            }
        }
    }

    auto Parser::get_int64(const SystemStringType& section, const SystemStringType& key) const -> int64_t
    {
        const auto maybe_value = get_value(section, key);
        if (!maybe_value.has_value())
        {
            throw std::runtime_error{"[Ini::get_int64] Tried getting value of type 'Int64' but the value didn't exist."};
        }
        else
        {
            const auto& value = maybe_value.value().get();
            if (!value.get_ref()->is_valid_int64())
            {
                throw std::runtime_error{"[Ini::get_int64] Tried getting value of type 'Int64' but the variable cannot be interpreted as 'Int64'"};
            }
            else
            {
                return value.get_ref()->get_int64_value();
            }
        }
    }

    auto Parser::get_float(const SystemStringType& section, const SystemStringType& key, float default_value) const noexcept -> float
    {
        const auto maybe_value = get_value(section, key, CanThrow::No);
        if (!maybe_value.has_value())
        {
            return default_value;
        }
        else
        {
            const auto& value = maybe_value.value().get();
            if (!value.get_ref()->is_valid_float())
            {
                return default_value;
            }
            else
            {
                return value.get_ref()->get_float_value();
            }
        }
    }

    auto Parser::get_float(const SystemStringType& section, const SystemStringType& key) const -> float
    {
        const auto maybe_value = get_value(section, key);
        if (!maybe_value.has_value())
        {
            throw std::runtime_error{"[Ini::get_float] Tried getting value of type 'Float' but the value didn't exist."};
        }
        else
        {
            const auto& value = maybe_value.value().get();
            if (!value.get_ref()->is_valid_float())
            {
                throw std::runtime_error{"[Ini::get_float] Tried getting value of type 'Float' but the variable cannot be interpreted as 'Float'"};
            }
            else
            {
                return value.get_ref()->get_float_value();
            }
        }
    }

    auto Parser::get_bool(const SystemStringType& section, const SystemStringType& key, bool default_value) const noexcept -> bool
    {
        const auto maybe_value = get_value(section, key, CanThrow::No);
        if (!maybe_value.has_value())
        {
            return default_value;
        }
        else
        {
            const auto& value = maybe_value.value().get();
            if (!value.get_ref()->is_valid_bool())
            {
                return default_value;
            }
            else
            {
                return value.get_ref()->get_bool_value();
            }
        }
    }

    auto Parser::get_bool(const SystemStringType& section, const SystemStringType& key) const -> bool
    {
        const auto maybe_value = get_value(section, key);
        if (!maybe_value.has_value())
        {
            throw std::runtime_error{"[Ini::get_int64] Tried getting value of type 'Bool' but the value didn't exist."};
        }
        else
        {
            const auto& value = maybe_value.value().get();
            if (!value.get_ref()->is_valid_bool())
            {
                throw std::runtime_error{"[Ini::get_bool] Tried getting value of type 'Bool' but the variable cannot be interpreted as 'Bool'"};
            }
            else
            {
                return value.get_ref()->get_bool_value();
            }
        }
    }
} // namespace RC::Ini
