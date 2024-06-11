#pragma once

#include <File/File.hpp>

namespace RC
{
    class UE4SSProgram;
}

namespace RC::UEGenerator::JSONDumper
{
    auto dump_to_json(SystemStringType file_name) -> void;
}
