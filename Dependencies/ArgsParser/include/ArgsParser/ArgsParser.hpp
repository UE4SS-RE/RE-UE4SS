#ifndef RC_ARGS_PARSER_HPP
#define RC_ARGS_PARSER_HPP

#include <vector>
#include <unordered_map>
#include <format>

namespace RC
{
    class ArgsParser
    {
        struct Arg
        {
            std::string arg_name{};
            std::string single_arg{};
            std::vector<std::string> multi_arg{};
            bool contains_multiple_arguments{};
        };

    private:
        int m_arg_count{};
        char** m_raw_args{};
        std::unordered_map<std::string, Arg> m_args{};
        const std::vector<std::string> m_accepted_arguments{};

    public:
        ArgsParser(int argc, char* argv[], const std::vector<std::string>& accepted_arguments) : m_arg_count(argc), m_raw_args(argv), m_accepted_arguments(accepted_arguments)
        {
            parse();
        }

    private:
        auto parse() -> void
        {
            /*
             * Format:
             *          Each argument must be separated by a space.
             *          Argument names cannot contain spaces.
             *          An argument must be formatted as such (no quotes): "-<arg-name>=<arg value>".
             *          The argument value needs to be surrounded in double-quotes if it contains spaces.
             *          The strict format of argument names will be used to deal with argument values that contain spaces.
             */

            for (int current_arg = 1; current_arg < m_arg_count; ++current_arg)
            {
                const auto arg = std::string{m_raw_args[current_arg]};

                //printf_s("arg: %s\n", arg.c_str());
                for (const auto& accepted_argument : m_accepted_arguments)
                {
                    if (auto arg_name_pos = arg.find(accepted_argument); arg_name_pos != arg.npos)
                    {
                        const auto arg_name = accepted_argument;
                        const auto arg_value = arg.substr(arg_name_pos + accepted_argument.size() + 1);

                        if (auto arg_it = m_args.find(arg_name); arg_it != m_args.end())
                        {
                            arg_it->second.contains_multiple_arguments = true;
                            arg_it->second.arg_name = arg_name;
                            arg_it->second.multi_arg.emplace_back(arg_value);
                            arg_it->second.single_arg.append(" ");
                            arg_it->second.single_arg.append(arg_value);
                        }
                        else
                        {
                            auto& arg_ref = m_args.emplace(arg_name, Arg{}).first->second;
                            arg_ref.arg_name = arg_name;

                            if (auto space_pos = arg_value.find(' '); space_pos != arg_value.npos)
                            {
                                arg_ref.contains_multiple_arguments = true;
                                arg_ref.single_arg = arg_value.substr(0, space_pos);
                                arg_ref.multi_arg.emplace_back(arg_ref.single_arg);

                                while (space_pos != arg_value.npos)
                                {
                                    auto next_space_pos = arg_value.find(' ', space_pos + 1);
                                    auto next_arg_value = arg_value.substr(space_pos + 1, next_space_pos - space_pos - 1);
                                    arg_ref.single_arg.append(" ");
                                    arg_ref.single_arg.append(next_arg_value);
                                    arg_ref.multi_arg.emplace_back(next_arg_value);
                                    space_pos = next_space_pos;
                                }
                            }
                            else
                            {
                                arg_ref.multi_arg.emplace_back(arg_value);
                                arg_ref.single_arg = arg_value;
                            }
                        }
                        break;
                    }
                }
            }

            /*
            printf_s("\n\n");
            for (const auto& [_, arg] : m_args)
            {
                if (arg.contains_multiple_arguments)
                {
                    printf_s("contains_multiple_arguments=yes\n");
                    if (!arg.single_arg.empty())
                    {
                        printf_s("single_arg: %s\n", arg.single_arg.c_str());
                    }

                    for (const auto& arg_value : arg.multi_arg)
                    {
                        printf_s("arg_value: %s\n", arg_value.c_str());
                    }
                }
                else
                {
                    printf_s("contains_multiple_arguments=no\n");
                    printf_s("single_arg: %s\n", arg.single_arg.c_str());
                }
            }
            //*/
        }

    public:
        auto get_arg(std::string arg_name) -> std::string
        {
            if (auto it = m_args.find(arg_name); it != m_args.end())
            {
                return it->second.single_arg;
            }
            else
            {
                return {};
            }
        }

        auto get_arg_as_vector(std::string arg_name) -> std::vector<std::string>
        {
            if (auto it = m_args.find(arg_name); it != m_args.end())
            {
                return it->second.multi_arg;
            }
            else
            {
                return {};
            }
        }
    };
}

#endif //RC_ARGS_PARSER_HPP
