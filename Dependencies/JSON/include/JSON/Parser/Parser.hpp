#ifndef RC_JSON_PARSER_HPP
#define RC_JSON_PARSER_HPP

#include <memory>

#include <File/File.hpp>
#include <JSON/JSON.hpp>
#include <ParserBase/Tokenizer.hpp>
#include <JSON/Parser/TokenParser.hpp>
//#include <JSON/Parser/Types.hpp>

namespace RC::JSON::Parser
{
    RC_JSON_API auto parse(StringType& input) -> std::unique_ptr<JSON::Object>;
    RC_JSON_API auto parse(const File::Handle&) -> std::unique_ptr<JSON::Object>;
}

#endif // RC_JSON_PARSER_HPP