#include <iostream>

#include <ArgsParser/ArgsParser.hpp>

using namespace RC;

auto main(int argc, char* argv[]) -> int
{
    printf_s("ArgsParser Test\n");

    try
    {
        ArgsParser args_parser{argc, argv, {
                "arg_one",
                "arg_two",
                "arg_three",
                "output",
                "sources",
                "compiler_flags",
        }};

        auto arg_one = args_parser.get_arg("arg_one");
        printf_s("arg_one: %s\n", arg_one.c_str());

        auto arg_two = args_parser.get_arg("arg_two");
        auto arg_two_as_vector = args_parser.get_arg_as_vector("arg_two");
        printf_s("arg_two: %s\n", arg_two.c_str());
        for (const auto& arg_two_val : arg_two_as_vector)
        {
            printf_s("arg_two_val: %s\n", arg_two_val.c_str());
        }

        auto arg_three = args_parser.get_arg("arg_three");
        printf_s("arg_three: %s\n", arg_three.c_str());

        auto arg_four = args_parser.get_arg("arg_four");
        auto arg_four_as_vector = args_parser.get_arg_as_vector("arg_four");
        printf_s("arg_four: %s\n", arg_four.empty() ? "empty" : arg_four.c_str());
        printf_s("arg_four_as_vector: %s\n", arg_four_as_vector.empty() ? "empty" : "not empty");

        auto output = args_parser.get_arg("output");
        printf_s("output: %s\n", output.c_str());

        auto sources = args_parser.get_arg_as_vector("sources");
        for (const auto& source : sources)
        {
            printf_s("source: %s\n", source.c_str());
        }

        auto compiler_flags = args_parser.get_arg_as_vector("compiler_flags");
        for (const auto& compiler_flag : compiler_flags)
        {
            printf_s("compiler_flag: %s\n", compiler_flag.c_str());
        }
    }
    catch (std::runtime_error& e)
    {
        printf_s("Error: %s\n", e.what());
        return 1;
    }

    return 0;
}
