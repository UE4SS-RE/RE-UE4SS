#pragma once

namespace RC::JSON::Parser
{
    enum TokenType : int
    {
        CarriageReturn,
        NewLine,
        DoubleQuote,
        Characters,
        ClosingCurlyBrace,
        OpeningCurlyBrace,
        ClosingSquareBracket,
        OpeningSquareBracket,
        Comma,
        Colon,
        True,
        False,
        EndOfFile,
    };

    auto inline token_type_to_string(const TokenType token_type) -> std::string_view
    {
        switch (token_type)
        {
        case CarriageReturn:
            return "CarriageReturn";
        case NewLine:
            return "NewLine";
        case DoubleQuote:
            return "DoubleQuote";
        case Characters:
            return "Characters";
        case ClosingCurlyBrace:
            return "ClosingCurlyBrace";
        case OpeningCurlyBrace:
            return "OpeningCurlyBrace";
        case ClosingSquareBracket:
            return "ClosingSquareBracket";
        case OpeningSquareBracket:
            return "OpeningSquareBracket";
        case Comma:
            return "Comma";
        case Colon:
            return "Colon";
        case True:
            return "True";
        case False:
            return "False";
        case EndOfFile:
            return "EndOfFile";
        }

        return "UnhandledToken";
    }
} // namespace RC::JSON::Parser
