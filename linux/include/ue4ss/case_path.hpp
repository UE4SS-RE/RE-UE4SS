#pragma once

#include <string>
#include <string_view>

namespace ue4ss::linux
{
    enum class PathResolutionError
    {
        none,
        invalid_component,
        not_found,
        ambiguous,
        io_error,
    };

    struct PathResolution
    {
        std::string path;
        std::string message;
        PathResolutionError error{PathResolutionError::none};

        [[nodiscard]] explicit operator bool() const
        {
            return error == PathResolutionError::none;
        }
    };

    [[nodiscard]] PathResolution resolve_case_insensitive(const std::string& root, std::string_view relative_path);
} // namespace ue4ss::linux
