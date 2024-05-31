#include <JSON/Parser/Parser.hpp>
#include <JSON/Parser/TokenParser.hpp>
#include <JSON/Parser/Tokens.hpp>
#include <ParserBase/Token.hpp>

namespace RC::JSON::Parser
{
    namespace Internal
    {
        static auto create_available_tokens_for_tokenizer() -> ParserBase::TokenContainer
        {
            ParserBase::TokenContainer tc;

            tc.add(ParserBase::Token::create(TokenType::CarriageReturn, STR("CarriageReturn"), STR("\r")));
            tc.add(ParserBase::Token::create(TokenType::NewLine, STR("NewLine"), STR("\n")));
            tc.add(ParserBase::Token::create(TokenType::DoubleQuote, STR("DoubleQuote"), STR("\"")));
            tc.add(ParserBase::Token::create(TokenType::Characters,
                                             STR("Characters"),
                                             STR(""),
                                             ParserBase::Token::HasData::Yes)); // Empty identifier will match everything that no other token identifier matches
            tc.add(ParserBase::Token::create(TokenType::ClosingCurlyBrace, STR("ClosingCurlyBrace"), STR("}")));
            tc.add(ParserBase::Token::create(TokenType::OpeningCurlyBrace, STR("OpeningCurlyBrace"), STR("{")));
            tc.add(ParserBase::Token::create(TokenType::ClosingSquareBracket, STR("ClosingSquareBracket"), STR("]")));
            tc.add(ParserBase::Token::create(TokenType::OpeningSquareBracket, STR("OpeningSquareBracket"), STR("[")));
            tc.add(ParserBase::Token::create(TokenType::Comma, STR("Comma"), STR(",")));
            tc.add(ParserBase::Token::create(TokenType::Colon, STR("Colon"), STR(":")));
            tc.add(ParserBase::Token::create(TokenType::True, STR("True"), STR("true")));
            tc.add(ParserBase::Token::create(TokenType::False, STR("False"), STR("false")));

            tc.set_eof_token(TokenType::EndOfFile);

            return tc;
        }

        static auto parse_internal(File::StringType& input) -> std::unique_ptr<JSON::Object>
        {
            // Tokenize -> START
            ParserBase::Tokenizer tokenizer;
            tokenizer.set_available_tokens(create_available_tokens_for_tokenizer());
            tokenizer.tokenize(input);
            // Tokenize -> END

            // Parse Tokens -> START
            TokenParser token_parser{tokenizer, input};
            token_parser.parse();
            std::unique_ptr<JSON::Object> global_object = token_parser.release_global_object();
            if (!global_object)
            {
                // Make an empty global object if we for some reason failed to parse any objects at all.
                global_object = std::make_unique<JSON::Object>();
            }
            return std::move(global_object);
            // Parse Tokens -> END
        }
    } // namespace Internal

    auto parse(File::StringType& input) -> std::unique_ptr<JSON::Object>
    {
        return Internal::parse_internal(input);
    }

    auto parse(const File::Handle& file) -> std::unique_ptr<JSON::Object>
    {
        auto input = file.read_all();
        return Internal::parse_internal(input);
    }
} // namespace RC::JSON::Parser
