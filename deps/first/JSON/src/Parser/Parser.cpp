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

            tc.add(ParserBase::Token::create(TokenType::CarriageReturn, SYSSTR("CarriageReturn"), SYSSTR("\r")));
            tc.add(ParserBase::Token::create(TokenType::NewLine, SYSSTR("NewLine"), SYSSTR("\n")));
            tc.add(ParserBase::Token::create(TokenType::DoubleQuote, SYSSTR("DoubleQuote"), SYSSTR("\"")));
            tc.add(ParserBase::Token::create(TokenType::Characters,
                                             SYSSTR("Characters"),
                                             SYSSTR(""),
                                             ParserBase::Token::HasData::Yes)); // Empty identifier will match everything that no other token identifier matches
            tc.add(ParserBase::Token::create(TokenType::ClosingCurlyBrace, SYSSTR("ClosingCurlyBrace"), SYSSTR("}")));
            tc.add(ParserBase::Token::create(TokenType::OpeningCurlyBrace, SYSSTR("OpeningCurlyBrace"), SYSSTR("{")));
            tc.add(ParserBase::Token::create(TokenType::ClosingSquareBracket, SYSSTR("ClosingSquareBracket"), SYSSTR("]")));
            tc.add(ParserBase::Token::create(TokenType::OpeningSquareBracket, SYSSTR("OpeningSquareBracket"), SYSSTR("[")));
            tc.add(ParserBase::Token::create(TokenType::Comma, SYSSTR("Comma"), SYSSTR(",")));
            tc.add(ParserBase::Token::create(TokenType::Colon, SYSSTR("Colon"), SYSSTR(":")));
            tc.add(ParserBase::Token::create(TokenType::True, SYSSTR("True"), SYSSTR("true")));
            tc.add(ParserBase::Token::create(TokenType::False, SYSSTR("False"), SYSSTR("false")));

            tc.set_eof_token(TokenType::EndOfFile);

            return tc;
        }

        static auto parse_internal(SystemStringType& input) -> std::unique_ptr<JSON::Object>
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
        auto sys_input = to_system(input);
        return Internal::parse_internal(sys_input);
    }

    auto parse(UEStringType& input) -> std::unique_ptr<JSON::Object>
    {
        auto sys_input = to_system(input);
        return Internal::parse_internal(sys_input);
    }

    auto parse(const File::Handle& file) -> std::unique_ptr<JSON::Object>
    {
        auto input = to_system_string(file.read_file_all());
        return Internal::parse_internal(input);
    }
} // namespace RC::JSON::Parser
