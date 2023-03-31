#include <fstream>
#include <sstream>

#include <ParserBase/Tokenizer.hpp>
#include <ParserBase/Token.hpp>

namespace RC::ParserBase
{
    auto TokenContainer::add(Token token) -> size_t
    {
        m_tokens.emplace_back(std::move(token));
        return m_tokens.size() - 1;
    }

    auto TokenContainer::set_eof_token(int token_type) -> void
    {
        m_eof_token_type = token_type;
        m_has_eof_token_type = true;
    }

    auto TokenContainer::get_all() -> const std::vector<Token>&
    {
        return m_tokens;
    }

    auto TokenContainer::get_by_type(int type) -> Token*
    {
        for (auto& token : m_tokens)
        {
            if (token.get_type() == type)
            {
                return &token;
            }
        }

        return nullptr;
    }

    auto Tokenizer::set_available_tokens(TokenContainer&& token_container) -> void
    {
        m_token_container = std::move(token_container);
    }

    auto Tokenizer::tokenize(const File::StringType& input) -> void
    {
        //printf_s("Tokenizer::tokenize()\n\n");

        if (!m_token_container.m_has_eof_token_type)
        {
            throw std::runtime_error{"Please call TokenContainer::set_of_token before attempting to tokenize"};
        }

        /*
        printf_s("Possible tokens\n");
        for (const auto& token : m_token_container.get_all())
        {
            printf_s("Token: %S\n", token.to_string().c_str());
            printf_s("Identifier: %S\n", token.get_identifier().data());

            if (!token.get_rules().empty())
            {
                printf_s("Rules\n");
                for (const auto& rule : token.get_rules())
                {
                    printf_s("    %S\n", rule->to_string().c_str());
                }
            }

            printf_s("\n");
        }
        //*/
        
        if (input.empty())
        {
            throw std::runtime_error{"[Tokenizer::tokenize] Input was empty"};
        }

        File::StringType a;

        const File::CharType* input_array = input.c_str();
        size_t global_cursor{};

        auto peek = [&](File::StringType& out_str, const File::CharType* character, size_t num_chars) -> void {
            if (global_cursor + num_chars <= input.size())
            {
                for (size_t i = 0; i < num_chars; ++i)
                {
                    out_str += *character;
                    ++character;
                }
            }
        };

        auto try_find_token_identifier = [&](const Token& token_to_find) -> int {
            //File::StringType b;
            int local_cursor{};

            for (const File::CharType* c = &input_array[global_cursor+1]; local_cursor < input.size() - global_cursor; ++c, ++local_cursor)
            {
                File::StringViewType identifier_to_find = token_to_find.get_identifier();
                File::StringType compare_to;
                peek(compare_to, c, identifier_to_find.size());

                if (identifier_to_find == compare_to)
                {
                    return local_cursor;
                }

                /*
                if (*c == identifier_to_find[0])
                {
                    // The first character matches the first character of the identifier
                    if (identifier_to_find.size() == 1)
                    {
                        // The size of the identifier to find is 1, then we've found the identifier, exit early
                        return local_cursor;
                    }

                    // Add it to 'b' and step forward
                    b += *c;
                }
                else if (b == identifier_to_find)
                {
                    return local_cursor;
                }
                else if (b.size() > 0)
                {
                    b.clear();
                }
                //*/
            }
            
            return -1;
        };

        struct TokenFoundWrapper
        {
            const Token* token{nullptr};
            bool matched_anything{false};
        };

        File::StringType current_empty_token{};
        File::StringType empty_token_data{};
        TokenFoundWrapper empty_token{};
        size_t start_of_empty_token{};
        bool start_of_empty_token_set{};
        const File::CharType* c = input_array;

        auto deal_with_possible_empty_token = [&]() {
            if (empty_token.token)
            {
                empty_token.token->m_start = start_of_empty_token;
                empty_token.token->m_end = global_cursor - 1;
                empty_token.token->m_line = m_current_line;
                empty_token.token->m_column = m_current_column + 1;
                m_tokens_in_input.emplace_back(*empty_token.token);
                empty_token_data.clear();
                empty_token = {};
                start_of_empty_token_set = false;
            }
        };

        for (; global_cursor < input.size(); ++c, ++global_cursor)
        {
            TokenFoundWrapper token_found{};

            if (c && *c == L'\n')
            {
                ++m_current_line;
                m_current_column = 0;
            }
            else
            {
                ++m_current_column;
            }

            for (auto& token : m_token_container.get_all())
            {
                int advance_cursor_by{-1};
                bool all_rules_obeyed{true};

                File::StringViewType identifier_to_find = token.get_identifier();
                File::StringType compare_to;
                size_t identifier_size = identifier_to_find.size();
                peek(compare_to, c, identifier_size);
                bool identifier_should_match_all = identifier_to_find.empty();

                // Empty identifier matches everything
                if (identifier_to_find == compare_to || identifier_should_match_all)
                {
                    for (const auto& rule : token.get_rules())
                    {
                        advance_cursor_by = rule->exec(token, c, global_cursor, *this);

                        if (advance_cursor_by == -1)
                        {
                            all_rules_obeyed = false;
                        }
                    }

                    if (all_rules_obeyed/* && !identifier_should_match_all*/)
                    {
                        // current_empty_token.clear();
                        token_found = {.token = &token, .matched_anything = identifier_should_match_all};
                        token_found.token->m_start = global_cursor;

                        if (identifier_size > 1)
                        {
                            advance_cursor_by += static_cast<int>(advance_cursor_by == -1 ? identifier_size : identifier_size - 1);
                        }

                        if (advance_cursor_by > 0)
                        {
                            c += advance_cursor_by;
                            global_cursor += advance_cursor_by;
                            m_current_column += advance_cursor_by;
                        }

                        token_found.token->m_end = global_cursor;

                        if (!identifier_should_match_all)
                        {
                            break;
                        }
                    }
                }
            }

            if (token_found.token)
            {
                if (token_found.matched_anything)
                {
                    if (!start_of_empty_token_set)
                    {
                        start_of_empty_token = global_cursor;
                        start_of_empty_token_set = true;
                    }

                    empty_token_data += *c;
                    empty_token = token_found;
                }
                else
                {
                    deal_with_possible_empty_token();

                    token_found.token->m_line = m_current_line;
                    token_found.token->m_column = m_current_column;
                    m_tokens_in_input.emplace_back(*token_found.token);
                    a.clear();
                }
            }
        }

        // Deal with the last token right before end-of-file
        // This is required because the loop above stops at end-of-file without processing it
        deal_with_possible_empty_token();

        m_tokens_in_input.emplace_back(Token{m_token_container.m_eof_token_type, STR("EndOfFile"), STR("--EOF-DO-NOT-PARSE--")});
    }

    auto Tokenizer::get_tokens() const -> const std::vector<Token>&
    {
        return m_tokens_in_input;
    }

    auto Tokenizer::get_last_token() const -> const Token&
    {
        return m_tokens_in_input.back();
    }
}
