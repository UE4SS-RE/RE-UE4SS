#ifndef RC_MPROGRAM_HPP
#define RC_MPROGRAM_HPP

#include <initializer_list>
#include <memory>

#include <ErrorObject.hpp>
#include <ProgramFeatures/CanError.hpp>

namespace RC
{
    class MProgram : public CanError
    {
    public:
        enum BinaryOptions : int
        {
            Nothing = 0,
            CloseOnError,

            Max
        };

    protected:
        bool m_binary_options[Max]{};

    protected:
        MProgram()
        {
            create_error_object();
        }

        MProgram(std::initializer_list<BinaryOptions> options)
        {
            for (auto& option: options)
            {
                m_binary_options[option] = true;
            }

            create_error_object();
        }

        ~MProgram() = default;

    public:
        auto should_close() -> bool
        {
            return m_binary_options[CloseOnError];
        }

    private:
        auto create_error_object() -> void
        {
            m_error_object = std::make_shared<ErrorObject>(m_binary_options[CloseOnError]);
        }
    };
}

#endif //RC_MPROGRAM_HPP
