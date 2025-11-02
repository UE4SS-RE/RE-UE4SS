#pragma once

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
