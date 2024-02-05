#include <JSON/Array.hpp>
#include <JSON/Bool.hpp>
#include <JSON/Null.hpp>
#include <JSON/Object.hpp>
#include <JSON/String.hpp>

namespace RC::JSON
{
    auto Object::find_value_by_key(const SystemStringType& look_for_key) const -> Value*
    {
        if (auto it = m_members.find(look_for_key); it != m_members.end())
        {
            return it->second.get();
        }
        else
        {
            return nullptr;
        }
    }
    auto Object::find_value_by_key(const SystemStringType& look_for_key) -> Value*
    {
        if (auto it = m_members.find(look_for_key); it != m_members.end())
        {
            return it->second.get();
        }
        else
        {
            return nullptr;
        }
    }

    auto Object::new_object(SystemStringType name) -> Object&
    {
        return *static_cast<Object*>(m_members.emplace(std::move(name), std::make_unique<Object>()).first->second.get());
    }

    auto Object::new_array(SystemStringType name) -> Array&
    {
        return *static_cast<Array*>(m_members.emplace(std::move(name), std::make_unique<Array>()).first->second.get());
    }

    auto Object::new_string(SystemStringType name, const SystemStringType& value) -> void
    {
        m_members.emplace(std::move(name), std::make_unique<String>(value));
    }

    auto Object::new_null(SystemStringType name) -> void
    {
        m_members.emplace(std::move(name), std::make_unique<Null>());
    }

    auto Object::new_bool(SystemStringType name, bool value) -> void
    {
        m_members.emplace(std::move(name), std::make_unique<Bool>(value));
    }

    auto Object::add_object(SystemStringType name, std::unique_ptr<Object> object) -> void
    {
        m_members.emplace(std::move(name), std::move(object));
    }

    auto Object::serialize(ShouldFormat should_format, int32_t* indent_level) -> SystemStringType
    {
        if (!indent_level)
        {
            throw std::runtime_error{"Must supply an indent_level pointer"};
        };

        SystemStringType object_as_string{};

        if (should_format == ShouldFormat::Yes && !m_members.empty())
        {
            object_as_string.append(SYSSTR("{\n"));
        }
        else
        {
            object_as_string.append(SYSSTR("{"));
        }

        ++(*indent_level);

        size_t member_count{};
        for (const auto& [key, value] : m_members)
        {
            if (should_format == ShouldFormat::Yes)
            {
                indent(indent_level, object_as_string);
            }

            object_as_string.append(std::format(SYSSTR("\"{}\":"), key));
            object_as_string.append(value->serialize(should_format, indent_level));

            if (member_count + 1 < m_members.size())
            {
                if (should_format == ShouldFormat::Yes)
                {
                    object_as_string.append(SYSSTR(",\n"));
                }
                else
                {
                    object_as_string.append(SYSSTR(","));
                }
            }

            ++member_count;
        }

        --(*indent_level);

        if (should_format == ShouldFormat::Yes && !m_members.empty())
        {
            object_as_string.append(SYSSTR("\n"));
            indent(indent_level, object_as_string);
            object_as_string.append(SYSSTR("}"));
        }
        else
        {
            object_as_string.append(SYSSTR("}"));
        }

        if (!m_members.empty() || m_is_global_object == IsGlobalObject::Yes)
        {
            // object_as_string.erase(object_as_string.end() - 2);
        }

        return object_as_string;
    }
} // namespace RC::JSON
