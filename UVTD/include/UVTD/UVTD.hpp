#ifndef RC_UVTD_HPP
#define RC_UVTD_HPP

#include <filesystem>
#include <unordered_set>
#include <set>
#include <map>
#include <utility>

#include <UVTD/Symbols.hpp>
#include <File/File.hpp>
#include <Input/Handler.hpp>
#include <Function/Function.hpp>
#include <DynamicOutput/DynamicOutput.hpp>

namespace RC::UVTD
{
    extern bool processing_events;
    extern Input::Handler input_handler;

    auto main(DumpMode) -> void;
}

#endif //RC_UVTD_HPP
