#pragma once

#include <memory>

#include <File/File.hpp>
#include <JSON/JSON.hpp>
#include <JSON/Parser/TokenParser.hpp>
#include <ParserBase/Tokenizer.hpp>
// #include <JSON/Parser/Types.hpp>

namespace RC::JSON::Parser
{
    RC_JSON_API auto parse(File::StringType& input) -> std::unique_ptr<JSON::Object>;
    RC_JSON_API auto parse(const File::Handle&) -> std::unique_ptr<JSON::Object>;
} // namespace RC::JSON::Parser
