#pragma once

#include <File/File.hpp>
#include <JSON/JSON.hpp>
#include <JSON/Parser/TokenParser.hpp>
#include <ParserBase/Tokenizer.hpp>

#include <memory>
// #include <JSON/Parser/Types.hpp>

namespace RC::JSON::Parser
{
    RC_JSON_API auto parse(StringType& input) -> std::unique_ptr<JSON::Object>;
    RC_JSON_API auto parse(const File::Handle&) -> std::unique_ptr<JSON::Object>;
} // namespace RC::JSON::Parser
