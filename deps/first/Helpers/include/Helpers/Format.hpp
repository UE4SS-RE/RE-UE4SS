#pragma once

#include <string>
#include <format>

#include <Helpers/String.hpp>
#include <DynamicOutput/Output.hpp>

namespace RC
{
    template <typename... FmtArgs>
    auto static fmt(const std::string_view&& fmt, FmtArgs&&... fmt_args) -> std::string
    {
        return Output::apply_formatting(fmt, to_stdstr(std::forward<FmtArgs>(fmt_args))...);
    }

    template <typename... FmtArgs>
    auto static fmtfile(const File::StringViewType&& fmt, FmtArgs&&... fmt_args) -> std::string
    {
        return Output::apply_formatting(fmt, to_file(std::forward<FmtArgs>(fmt_args))...);
    }
} // namespace RC