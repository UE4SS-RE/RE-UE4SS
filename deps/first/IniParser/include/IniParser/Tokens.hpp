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
    Plus,
    Minus,
    EndOfFile,
};
using IniTokenType = TokenType;
