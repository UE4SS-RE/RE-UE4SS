#ifndef UE4SS_REWRITTEN_TESTDEVICE_HPP
#define UE4SS_REWRITTEN_TESTDEVICE_HPP

#include <DynamicOutput/Macros.hpp>
#include <DynamicOutput/OutputDevice.hpp>

#if _WIN32
#define RC_TEST_DEVICE_PRINT printf_s
#elif __linux__
#define RC_TEST_DEVICE_PRINT printf
#endif

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
                RC_TEST_DEVICE_PRINT("Optional Arg: ValueDefault - ");
                break;
            case OptionalArgTest::ValueOne:
                RC_TEST_DEVICE_PRINT("Optional Arg: ValueOne - ");
                break;
            case OptionalArgTest::ValueTwo:
                RC_TEST_DEVICE_PRINT("Optional Arg: ValueTwo - ");
                break;
            case OptionalArgTest::ValueThree:
                RC_TEST_DEVICE_PRINT("Optional Arg: ValueThree - ");
                break;
            }

#if ENABLE_OUTPUT_DEVICE_DEBUG_MODE
            RC_DEVICE_PRINT_FUNC(fmt, optional_arg, "TestDevice received: ");
#else
            RC_DEVICE_PRINT_FUNC(fmt, optional_arg, "");
#endif
        }
    };
} // namespace RC::Output

#endif // UE4SS_REWRITTEN_TESTDEVICE_HPP
