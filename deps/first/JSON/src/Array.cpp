#include <JSON/Array.hpp>
#include <JSON/Bool.hpp>
#include <JSON/Null.hpp>
#include <JSON/Object.hpp>
#include <JSON/String.hpp>

namespace RC::JSON
{
    auto Array::new_object() -> Object&
    {
        return static_cast<Object&>(*m_members.emplace_back(std::make_unique<Object>()));
    }

    auto Array::new_array() -> class Array&
    {
        return static_cast<Array&>(*m_members.emplace_back(std::make_unique<Array>()));
    }

    JSON_DEPRECATED auto Array::new_string(const StringType& value) -> void
    {
        m_members.emplace_back(std::make_unique<String>(value));
    }

    auto Array::new_null() -> void
    {
        m_members.emplace_back(std::make_unique<Null>());
    }

    auto Array::new_bool(bool value) -> void
    {
        m_members.emplace_back(std::make_unique<Bool>(value));
    }

    auto Array::add_object(std::unique_ptr<Object> object) -> void
    {
        m_members.emplace_back(std::move(object));
    }

    auto Array::serialize(ShouldFormat should_format, int32_t* indent_level) -> StringType
    {
        if (!indent_level)
        {
            throw std::runtime_error{"Must supply an indent_level pointer"};
        };

        StringType array_as_string{};

        if (should_format == ShouldFormat::Yes && !m_members.empty())
        {
            array_as_string.append(STR("[\n"));
        }
        else
        {
            array_as_string.append(STR("["));
        }

        ++(*indent_level);

        size_t member_count{};
        for (const auto& member : m_members)
        {
            if (should_format == ShouldFormat::Yes)
            {
                indent(indent_level, array_as_string);
            }

            array_as_string.append(member->serialize(should_format, indent_level));

            if (member_count + 1 < m_members.size())
            {
                if (should_format == ShouldFormat::Yes)
                {
                    array_as_string.append(STR(",\n"));
                }
                else
                {
                    array_as_string.append(STR(","));
                }
            }

            ++member_count;
        }

        --(*indent_level);

        if (should_format == ShouldFormat::Yes && !m_members.empty())
        {
            array_as_string.append(STR("\n"));
            indent(indent_level, array_as_string);
            array_as_string.append(STR("]"));
        }
        else
        {
            array_as_string.append(STR("]"));
        }

        if (!m_members.empty())
        {
            // array_as_string.erase(array_as_string.end() - 2);
        }

        return array_as_string;
    }
} // namespace RC::JSON
