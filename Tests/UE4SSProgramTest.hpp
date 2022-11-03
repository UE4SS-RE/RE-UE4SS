#ifndef UE4SS_REWRITTEN_UE4SSPROGRAMTEST_HPP
#define UE4SS_REWRITTEN_UE4SSPROGRAMTEST_HPP

#include <UE4SSProgram.hpp>

namespace RC
{
    class UE4SSProgramTest : public UE4SSProgram
    {
    public:
        UE4SSProgramTest(const std::wstring& moduleFilePath, std::initializer_list<BinaryOptions> options);

    private:
        auto static execute_tests() -> void;
        auto static execute_function_call_tests() -> void;
    };
}


#endif //UE4SS_REWRITTEN_UE4SSPROGRAMTEST_HPP
