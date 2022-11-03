#ifndef UE4SS_REWRITTEN_JSONDUMPER_HPP
#define UE4SS_REWRITTEN_JSONDUMPER_HPP

#include <File/File.hpp>

namespace RC
{
    class UE4SSProgram;
}

namespace RC::UEGenerator::JSONDumper
{
    auto dump_to_json(File::StringViewType file_name) -> void;
}

#endif //UE4SS_REWRITTEN_JSONDUMPER_HPP
