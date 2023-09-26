#pragma once

#include <stdexcept>

namespace RC::File
{
    class FileNotFoundException : public std::runtime_error
    {
      public:
        explicit FileNotFoundException(const char* msg) : std::runtime_error(msg)
        {
        }
        explicit FileNotFoundException(const std::string& msg) : std::runtime_error(msg)
        {
        }
    };
} // namespace RC::File
