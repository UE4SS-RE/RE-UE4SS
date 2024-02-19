#pragma once

#include <string>
#include <format>

namespace RC
{
    template <typename... FmtArgs>
    auto static fmt(const std::string_view&& fmt, FmtArgs&&... fmt_args) -> std::string
    {
        return std::vformat(std::forward<const std::string_view>(fmt), RC_STD_MAKE_FORMAT_ARGS(to_file(std::forward<FmtArgs>(fmt_args))...));
    }
}