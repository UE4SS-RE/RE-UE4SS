#include <ParserBase/TokenParser.hpp>
#include <ParserBase/Tokenizer.hpp>
#include <ParserBase/Token.hpp>

namespace RC::ParserBase
{
    TokenParser::TokenParser(const Tokenizer& tokenizer, File::StringType& input) : m_tokenizer(tokenizer), m_data(input)
    {
    }

    auto TokenParser::get_token(size_t index) const -> const Token&
    {
        return m_tokenizer.get_tokens()[index];
    }

    auto TokenParser::get_data(const Token& token) const -> const File::StringType
    {
        if (!token.has_data())
        {
            throw std::runtime_error{ "Tried retrieving data of a token that doesn't have any data" };
        }

        auto data = m_data.substr(token.get_start(), token.get_end() - token.get_start() + 1);
        data.erase(std::find(data.begin(), data.end(), STR('\0')), data.end());
        return data;
    }

    /*
    auto TokenParser::find_token_with_data(int token_type, File::StringViewType data) const -> std::optional<std::reference_wrapper<const Token>>
    {
        for (const auto& token : m_tokenizer.get_tokens())
        {
            if (!token.has_data()) { continue; }
            if (token.get_type() != token_type) { continue; }

            if (get_data(token) == data)
            {
                return std::ref(token);
            }
        }

        return std::nullopt;
    }
    //*/

    auto TokenParser::get_start_token_index_ref(PeekDirection peek_direction, Consume consume, size_t& fallback) const -> size_t&
    {
        if (peek_direction == PeekDirection::Backward)
        {
            return m_backward_token_index;
        }
        else if (consume == Consume::Yes)
        {
            return m_current_token_index_being_parsed;
        }
        else
        {
            return fallback;
        }
    }

    auto TokenParser::peek(int num_tokens_to_peek, PeekDirection peek_direction) const -> const Token&
    {
        int offset = peek_direction == PeekDirection::Forward ? num_tokens_to_peek : -num_tokens_to_peek;
        const auto& tokens = m_tokenizer.get_tokens();
        const size_t num_tokens = tokens.size();
        const size_t next_token_index = get_start_token_index_ref(peek_direction, Consume::No, m_current_token_index_being_parsed) + offset;

        if (next_token_index > num_tokens - 1)
        {
            // Out-of-bounds, not invalid state but no token exists here
            return m_tokenizer.get_last_token();
        }
        else
        {
            return tokens[next_token_index];
        }
    }

    auto TokenParser::peek(PeekDirection peek_direction) const -> const Token&
    {
        return peek(1, peek_direction);
    }

    auto TokenParser::consume(int num_tokens_to_peek, PeekDirection peek_direction) const -> const Token&
    {
        const auto& token = peek(num_tokens_to_peek, peek_direction);

        if (peek_direction == PeekDirection::Backward)
        {
            --m_backward_token_index;
        }
        else
        {
            ++m_current_token_index_being_parsed;
        }

        return token;
    }

    auto TokenParser::consume(PeekDirection peek_direction) const -> const Token&
    {
        return consume(1, peek_direction);
    }

    auto TokenParser::peek_continually(PeekUntilCallable callable) const -> bool
    {
        return peek_continually_internal(Consume::No, callable);
    };

    auto TokenParser::consume_continually(PeekUntilCallable callable) const -> bool
    {
        return peek_continually_internal(Consume::Yes, callable);
    }

    /*
    auto TokenParser::peek_and_ignore_until(int type) const -> std::optional<std::reference_wrapper<const Token>>
    {
        //return std::ref(m_tokenizer.get_tokens()[0]);
        //return std::nullopt;

        //for (const auto& token : m_tokenizer.get_tokens())
        auto& tokens = m_tokenizer.get_tokens();
        size_t num_tokens = tokens.size();
        size_t next_token_index = m_current_token_index + 1;
        for (size_t i = next_token_index; i < num_tokens; ++i)
        {
            if (tokens[i].get_type() == type)
            {
                return std::ref(tokens[i]);
            }
        }

        return std::nullopt;
    }
    //*/

    auto TokenParser::calc_next_token_offset(size_t current_offset, PeekDirection peek_direction) const -> size_t
    {
        switch (peek_direction)
        {
        case TokenParser::PeekDirection::Forward:
            return ++current_offset;
        case TokenParser::PeekDirection::Backward:
            return --current_offset;
        }

        throw std::runtime_error{ "[calc_next_token_offset] Invalid PeekDirection" };
    }
    
    auto TokenParser::peek_until_internal(Consume consume, const std::vector<int>& find_types, PeekUntilCallable callable, PeekDirection peek_direction) const -> void
    {
        const auto& tokens = m_tokenizer.get_tokens();
        size_t num_tokens = tokens.size();

        size_t peek_i{ m_current_token_index_being_parsed };
        size_t& i = get_start_token_index_ref(peek_direction, consume, peek_i);

        size_t next_token_index = calc_next_token_offset(i, peek_direction);
        for (i = next_token_index; i < num_tokens; i = calc_next_token_offset(i, peek_direction))
        {
            bool master_break{};
            const auto& token = tokens[i];

            for (const auto& find_type : find_types)
            {
                if (callable(token) || token.get_type() == find_type)
                {
                    master_break = true;
                    break;
                }
            }

            if (master_break) { break; }
        }
    }

    auto TokenParser::peek_and_ignore_until_internal(Consume consume, const std::vector<int>& find_types, PeekDirection peek_direction) const -> const Token&
    {
        const auto& tokens = m_tokenizer.get_tokens();
        size_t num_tokens = tokens.size();

        size_t peek_i{m_current_token_index_being_parsed};
        size_t& i = get_start_token_index_ref(peek_direction, consume, peek_i);

        size_t next_token_index = calc_next_token_offset(i, peek_direction);
        for (i = next_token_index; i < num_tokens; i = calc_next_token_offset(i, peek_direction))
        {
            const auto& token = tokens[i];

            for (const auto& find_type : find_types)
            {
                if (token.get_type() == find_type)
                {
                    return token;
                }
            }
        }

        return m_tokenizer.get_last_token();
    }

    auto TokenParser::peek_continually_internal(Consume consume, PeekUntilCallable callable) const -> bool
    {
        const auto& tokens = m_tokenizer.get_tokens();
        size_t num_tokens = tokens.size();

        size_t peek_i{m_current_token_index_being_parsed};
        size_t& i = get_start_token_index_ref(PeekDirection::Forward, consume, peek_i);

        bool return_value{};
        size_t next_token_index = calc_next_token_offset(i, PeekDirection::Forward);
        for (i = next_token_index; i < num_tokens; i = calc_next_token_offset(i, PeekDirection::Forward))
        {
            const auto& token = tokens[i];

            return_value = callable(token);
            if (return_value)
            {
                break;
            }
        }

        return return_value;
    }

    auto TokenParser::peek_until(const int find_type, PeekUntilCallable callable, PeekDirection  peek_direction) const -> void
    {
        peek_until(std::vector<int>{find_type}, callable, peek_direction);
    }

    auto TokenParser::peek_until(const std::vector<int>& find_types, PeekUntilCallable callable, PeekDirection peek_direction) const -> void
    {
        peek_until_internal(Consume::No, find_types, callable, peek_direction);
    }

    auto TokenParser::consume_until(const int find_type, PeekUntilCallable callable, PeekDirection  peek_direction) const -> void
    {
        consume_until(std::vector<int>{find_type}, callable, peek_direction);
    }

    auto TokenParser::consume_until(const std::vector<int>& find_types, PeekUntilCallable callable, PeekDirection peek_direction) const -> void
    {
        peek_until_internal(Consume::Yes, find_types, callable, peek_direction);
    }

    auto TokenParser::peek_and_ignore_until(const int find_type, PeekDirection peek_direction) const -> const Token&
    {
        return peek_and_ignore_until(std::vector<int>{find_type}, peek_direction);
    }

    auto TokenParser::peek_and_ignore_until(const std::vector<int>& find_types, PeekDirection peek_direction) const -> const Token&
    {
        return peek_and_ignore_until_internal(Consume::No, find_types, peek_direction);
    }

    auto TokenParser::consume_and_ignore_until(const int find_type, PeekDirection peek_direction) const -> const Token&
    {
        return consume_and_ignore_until(std::vector<int>{find_type}, peek_direction);
    }

    auto TokenParser::consume_and_ignore_until(const std::vector<int>& find_types, PeekDirection peek_direction) const -> const Token&
    {
        return peek_and_ignore_until_internal(Consume::Yes, find_types, peek_direction);
    }

    auto TokenParser::parse() -> void
    {
        const auto& tokens = m_tokenizer.get_tokens();
        for (m_current_token_index_being_parsed = 0; m_current_token_index_being_parsed < tokens.size(); ++m_current_token_index_being_parsed)
        {
            m_backward_token_index = m_current_token_index_being_parsed;
            parse_token(tokens[m_current_token_index_being_parsed]);
        }
    }
}
