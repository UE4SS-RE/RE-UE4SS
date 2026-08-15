#include "ue4ss/case_path.hpp"

#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <sstream>
#include <vector>

namespace
{
    [[nodiscard]] char ascii_lower(char character)
    {
        if (character >= 'A' && character <= 'Z')
        {
            return static_cast<char>(character + ('a' - 'A'));
        }
        return character;
    }

    [[nodiscard]] bool ascii_case_equal(std::string_view left, std::string_view right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (std::size_t index = 0; index < left.size(); ++index)
        {
            if (ascii_lower(left[index]) != ascii_lower(right[index]))
            {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] std::vector<std::string> split_path(std::string_view path)
    {
        std::vector<std::string> components;
        std::size_t cursor = 0;
        while (cursor < path.size())
        {
            while (cursor < path.size() && path[cursor] == '/')
            {
                ++cursor;
            }
            const std::size_t start = cursor;
            while (cursor < path.size() && path[cursor] != '/')
            {
                ++cursor;
            }
            if (cursor > start)
            {
                components.emplace_back(path.substr(start, cursor - start));
            }
        }
        return components;
    }
} // namespace

namespace ue4ss::linux
{
    PathResolution resolve_case_insensitive(const std::string& root, std::string_view relative_path)
    {
        if (relative_path.empty() || (!relative_path.empty() && relative_path.front() == '/'))
        {
            return {.message = "path must be non-empty and relative", .error = PathResolutionError::invalid_component};
        }

        std::string current = root;
        for (const std::string& component : split_path(relative_path))
        {
            if (component == "." || component == ".." || component.find('\0') != std::string::npos)
            {
                return {.message = "unsafe path component: " + component, .error = PathResolutionError::invalid_component};
            }

            DIR* directory = opendir(current.c_str());
            if (directory == nullptr)
            {
                return {
                        .message = "cannot inspect " + current + ": " + std::strerror(errno),
                        .error = PathResolutionError::io_error,
                };
            }

            std::vector<std::string> matches;
            errno = 0;
            while (dirent* entry = readdir(directory))
            {
                if (ascii_case_equal(entry->d_name, component))
                {
                    matches.emplace_back(entry->d_name);
                }
            }
            const int read_error = errno;
            closedir(directory);
            if (read_error != 0)
            {
                return {
                        .message = "cannot read " + current + ": " + std::strerror(read_error),
                        .error = PathResolutionError::io_error,
                };
            }
            if (matches.empty())
            {
                return {
                        .message = "component not found under " + current + ": " + component,
                        .error = PathResolutionError::not_found,
                };
            }
            if (matches.size() > 1u)
            {
                std::ostringstream message;
                message << "ambiguous case variants under " << current << ':';
                for (const auto& match : matches)
                {
                    message << ' ' << match;
                }
                return {.message = message.str(), .error = PathResolutionError::ambiguous};
            }
            current += '/';
            current += matches.front();
        }
        return {.path = std::move(current)};
    }
} // namespace ue4ss::linux
