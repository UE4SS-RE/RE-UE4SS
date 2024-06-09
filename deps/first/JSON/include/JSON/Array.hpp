#pragma once

#include <concepts>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <Constructs/Loop.hpp>
#include <File/Macros.hpp>
#include <JSON/Common.hpp>
#include <JSON/Number.hpp>
#include <JSON/Value.hpp>

namespace RC::JSON
{
    template <typename F>
    concept CallableWithIndex = std::invocable<F&, size_t, Value&>;

    template <typename F>
    concept CallableWithoutIndex = std::invocable<F&, Value&>;

    class RC_JSON_API Array : public Value
    {
      public:
        constexpr static Type static_type = Type::Array;

#pragma warning(disable : 4251)
      private:
        std::vector<std::unique_ptr<Value>> m_members{};
#pragma warning(default : 4251)

      public:
        Array() = default;

        // Explicitly making the class non-copyable to enable dllexport with unique_ptr
        Array(const Array&) = delete;
        auto operator=(const Array&) -> Array& = delete;
        ~Array() override = default;

      public:
        auto new_object() -> class Object&;
        auto new_array() -> class Array&;
        auto new_string(const SystemStringType& value) -> void;
        auto new_null() -> void;
        auto new_bool(bool value) -> void;

        auto add_object(std::unique_ptr<Object>) -> void;

        template <JSONNumber number_type>
        auto new_number(number_type value) -> void
        {
            m_members.emplace_back(std::make_unique<Number>(value));
        }

        auto get() -> decltype(m_members)&
        {
            return m_members;
        };
        auto get() const -> const decltype(m_members)&
        {
            return m_members;
        };

        template <typename Callable>
        auto for_each(Callable callable) const -> void
        {
            for (decltype(m_members)::size_type i = 0; i < m_members.size(); ++i)
            {
                const auto& element = m_members[i];
                static_assert(CallableWithIndex<Callable> || CallableWithoutIndex<Callable>, "Callable must have params Value&, or size_t and Value&");
                if constexpr (CallableWithIndex<Callable>)
                {
                    if (callable(i, *element.get()) == LoopAction::Break)
                    {
                        break;
                    }
                }
                else
                {
                    if (callable(*element.get()) == LoopAction::Break)
                    {
                        break;
                    }
                }
            }
        }

      public:
        auto serialize(ShouldFormat should_format = ShouldFormat::No, int32_t* indent_level = nullptr) -> SystemStringType override;
        auto get_type() const -> Type override
        {
            return Type::Array;
        }
    };
} // namespace RC::JSON
