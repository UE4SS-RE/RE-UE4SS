#ifndef UNREALVTABLEDUMPER_TEMPLATECLASSPARSER_HPP
#define UNREALVTABLEDUMPER_TEMPLATECLASSPARSER_HPP

#include <vector>

#include <File/File.hpp>

namespace RC::UVTD
{
    struct ParsedTemplateClass
    {
        File::StringType class_name;
        std::vector<File::StringType> template_args;
    };

    class TemplateClassParser
    {
      public:
        static ParsedTemplateClass Parse(File::StringViewType input);
    };
} // namespace RC::UVTD

#endif