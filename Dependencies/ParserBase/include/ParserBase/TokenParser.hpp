#ifndef RC_PARSER_TOKENPARSER_HPP
#define RC_PARSER_TOKENPARSER_HPP

#include <vector>
#include <optional>
#include <string>
#include <functional>

#include <File/Macros.hpp>
#include <ParserBase/Common.hpp>

namespace RC::ParserBase
{
    using OrDoCallable = void(*)();
    class TokenParser
    {
    public:
        enum class Operation
        {
            ContinueAsNormal,
            SkipNextToken,
        };

        enum class PeekDirection
        {
            Forward,
            Backward,
        };

        enum class Consume
        {
            Yes,
            No,
        };

    private:
        const class Tokenizer& m_tokenizer;
        mutable File::StringType m_data;

    protected:
        mutable size_t m_current_token_index_being_parsed{0};
        mutable size_t m_backward_token_index{0};

    public:
        // Investigate whether I want to std::move the input here
        RC_PB_API TokenParser(const class Tokenizer&, File::StringType& input);

    protected:
        RC_PB_API virtual auto parse_token(const class Token& token) -> void = 0;

    private:
        RC_PB_API auto calc_next_token_offset(size_t current_offset, PeekDirection) const -> size_t;
        RC_PB_API auto get_start_token_index_ref(PeekDirection peek_direction, Consume consume, size_t& fallback) const -> size_t&;
        //using PeekUntilCallable = bool(*)(const Token&);
        using PeekUntilCallable = std::function<bool(const Token&)>;
        RC_PB_API auto peek_until_internal(Consume consume, const std::vector<int>& find_types, PeekUntilCallable, PeekDirection = PeekDirection::Forward) const -> void;
        RC_PB_API auto peek_and_ignore_until_internal(Consume consume, const std::vector<int>& find_types, PeekDirection peek_direction) const -> const class Token&;
        RC_PB_API auto peek_continually_internal(Consume consume, PeekUntilCallable) const -> bool;

    protected:
        RC_PB_API auto get_token(size_t index) const -> const class Token&;
        RC_PB_API auto get_data(const class Token&) const -> const File::StringType;

        //auto find_token_with_data(int token_type, File::StringViewType data) const -> std::optional<std::reference_wrapper<const class Token>>;

        // Explanation for this particular implementation of peak/consume
        // When you peek, you aren't advancing the cursor
        // The cursor will advance to the next token like normal
        // When you consume, you are advancing the cursor
        // The cursor will advance to token after the token you advanced to
        RC_PB_API auto peek(int num_tokens_to_peek, PeekDirection = PeekDirection::Forward) const -> const class Token&;
        RC_PB_API auto peek(PeekDirection = PeekDirection::Forward) const -> const class Token&;
        RC_PB_API auto consume(int num_tokens_to_peek, PeekDirection = PeekDirection::Forward) const -> const class Token&;
        RC_PB_API auto consume(PeekDirection = PeekDirection::Forward) const -> const class Token&;

        // Special peek/consume functions that only go forwards and that takes a callable and forwards the return value from the callable
        RC_PB_API auto peek_continually(PeekUntilCallable) const -> bool;
        RC_PB_API auto consume_continually(PeekUntilCallable) const -> bool;

        RC_PB_API auto peek_until(const int find_type, PeekUntilCallable, PeekDirection = PeekDirection::Forward) const -> void;
        RC_PB_API auto peek_until(const std::vector<int>& find_types, PeekUntilCallable, PeekDirection = PeekDirection::Forward) const -> void;
        RC_PB_API auto consume_until(const int find_type, PeekUntilCallable, PeekDirection = PeekDirection::Forward) const -> void;
        RC_PB_API auto consume_until(const std::vector<int>& find_types, PeekUntilCallable, PeekDirection = PeekDirection::Forward) const -> void;

        // Peeks/consumes tokens until either, A: there are no more tokens, or B: a token of type 'find_type' is found
        // If A, an empty optional is returned
        // If B, an optional containing the found token of type 'find_type' is returned
        RC_PB_API auto peek_and_ignore_until(const int find_type, PeekDirection = PeekDirection::Forward) const -> const class Token&;
        RC_PB_API auto peek_and_ignore_until(const std::vector<int>& find_types, PeekDirection = PeekDirection::Forward) const -> const class Token&;
        RC_PB_API auto consume_and_ignore_until(const int find_type, PeekDirection = PeekDirection::Forward) const -> const class Token&;
        RC_PB_API auto consume_and_ignore_until(const std::vector<int>& find_types, PeekDirection = PeekDirection::Forward) const -> const class Token&;

    public:
        RC_PB_API auto parse() -> void;
    };
}

#endif // RC_PARSER_TOKENPARSER_HPP
