#include <UVTD/TemplateClassParser.hpp>

namespace RC::UVTD
{
    ParsedTemplateClass TemplateClassParser::Parse(UEStringViewType input)
    {
        ParsedTemplateClass parsed{};

        for (const UEStringType::value_type character : input)
        {
            if (character == STR('<')) break;
            parsed.class_name += character;
        }

        size_t nesting_level = 0;
        UEStringType current_param{};

        for (size_t i = parsed.class_name.size(); i < input.size(); i++)
        {
            const UEStringType::value_type character = input[i];
            bool is_nesting_character = false;

            if (character == STR('>') && nesting_level == 1) break;

            if (character == STR('(') || character == STR('<'))
            {
                nesting_level++;
                if (nesting_level == 1) continue;
            }
            else if (character == STR(')') || character == STR('>'))
            {
                nesting_level--;
                if (nesting_level + 1 == 1) continue;
            }

            if (character == STR(',') && nesting_level == 1)
            {
                parsed.template_args.push_back(current_param);
                current_param = {};
                continue;
            }

            current_param += character;
        }

        if (!current_param.empty()) parsed.template_args.push_back(current_param);

        return parsed;
    }
} // namespace RC::UVTD