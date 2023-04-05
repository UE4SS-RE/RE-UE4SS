#ifndef UE4SS_REWRITTEN_TESTDEVICE_HPP
#define UE4SS_REWRITTEN_TESTDEVICE_HPP

#include <DynamicOutput/OutputDevice.hpp>
#include <DynamicOutput/Macros.hpp>

namespace RC::Output
{
    class TestDevice : public OutputDevice
    {
    public:
        enum class OptionalArgTest
        {
            ValueDefault,
            ValueOne,
            ValueTwo,
            ValueThree,
        };

    public:
#if ENABLE_OUTPUT_DEVICE_DEBUG_MODE
        TestDevice()
        {
            fmt::print("TestDevice opening...\n");
        }

        ~TestDevice() override
        {
            fmt::print("TestDevice closing...\n");
        }
#else
        ~TestDevice() override = default;
#endif

    public:
        auto has_optional_arg() const -> bool override
        {
            return true;
        }

        auto receive(File::StringViewType fmt) const -> void override
        {
            receive_with_optional_arg(fmt, 0);
        }

        auto receive_with_optional_arg(File::StringViewType fmt, int32_t optional_arg) const -> void override
        {
            OptionalArgTest typed_optional_arg = static_cast<OptionalArgTest>(optional_arg);
            switch (typed_optional_arg)
            {
                case OptionalArgTest::ValueDefault:
                    printf_s("Optional Arg: ValueDefault - ");
                    break;
                case OptionalArgTest::ValueOne:
                    printf_s("Optional Arg: ValueOne - ");
                    break;
                case OptionalArgTest::ValueTwo:
                    printf_s("Optional Arg: ValueTwo - ");
                    break;
                case OptionalArgTest::ValueThree:
                    printf_s("Optional Arg: ValueThree - ");
                    break;
            }

#if ENABLE_OUTPUT_DEVICE_DEBUG_MODE
            printf_s("TestDevice received: %S", fmt.c_str());
#else
            printf_s("%S", fmt.data());
#endif
        }
    };
}


#endif //UE4SS_REWRITTEN_TESTDEVICE_HPP
