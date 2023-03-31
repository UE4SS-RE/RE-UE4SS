#ifndef RC_INI_PARSER_TOKENS_HPP
#define RC_INI_PARSER_TOKENS_HPP

enum TokenType : int
{
    CarriageReturn,
    NewLine,
    Space,
    Characters,
    Equals,
    ClosingSquareBracket,
    OpeningSquareBracket,
    SemiColon,
    EndOfFile,
};
using IniTokenType = TokenType;

#endif //RC_INI_PARSER_TOKENS_HPP
