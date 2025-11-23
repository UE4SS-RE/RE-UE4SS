#pragma once

#include <filesystem>
#include <map>
#include <set>
#include <unordered_set>
#include <utility>

#include <DynamicOutput/DynamicOutput.hpp>
#include <File/File.hpp>
#include <Input/Handler.hpp>
#include <UVTD/Symbols.hpp>

namespace RC::UVTD
{
    extern bool processing_events;

    auto main(DumpSettings) -> void;
} // namespace RC::UVTD // RC_UVTD_HPP
