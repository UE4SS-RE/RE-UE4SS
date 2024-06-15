#ifndef UNREALVTABLEDUMPER_TEMPLATECLASSPARSER_HPP
#define UNREALVTABLEDUMPER_TEMPLATECLASSPARSER_HPP

#include <vector>

#include <File/File.hpp>

namespace RC::UVTD
{
    struct ParsedTemplateClass
    {
        UEStringType class_name;
        std::vector<UEStringType> template_args;
    };

    class TemplateClassParser
    {
      public:
        static ParsedTemplateClass Parse(UEStringViewType input);
    };
} // namespace RC::UVTD

#endif