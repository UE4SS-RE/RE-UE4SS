#pragma once

#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <variant>
#include <functional>
#include <unordered_map>

#include <imgui.h>
#include <String/StringType.hpp>
#include <Helpers/String.hpp>

namespace RC::GUI
{
    // Forward declarations
    class ImGuiValueContainer;

    // Callback types
    using ValueChangedCallback = std::function<void()>;
    using ValueAppliedCallback = std::function<void()>;
    using ExternalUpdateCallback = std::function<void()>;

    // Value source - who last updated the value
    enum class ValueSource
    {
        User,      // User changed via UI
        External,  // External system (e.g., game engine)
        Default    // Initial/default value
    };

    // Edit mode for values
    enum class EditMode
    {
        Editable,     // Normal editable control
        ReadOnly,     // Display only, no editing
        ViewOnly      // Minimal display, no controls
    };

    // Base interface for all ImGui value types
    class IImGuiValue
    {
    public:
        using Ptr = std::unique_ptr<IImGuiValue>;

        virtual ~IImGuiValue() = default;
        
        // Draw the control and return true if value changed
        virtual bool draw(const char* label = nullptr) = 0;
        virtual bool draw(const CharType* label = nullptr) = 0;
        
        // Draw just the value as text (read-only)
        virtual void draw_value(const char* label = nullptr) = 0;
        virtual void draw_value(const CharType* label = nullptr) = 0;
        
        // String conversion
        virtual void set_from_string(const std::string& value) = 0;
        virtual std::string get_as_string() const = 0;

        // Container support
        virtual bool has_pending_changes() const = 0;
        virtual void apply_changes() = 0;
        virtual void revert_changes() = 0;
        virtual void mark_dirty() = 0;
        
        // Metadata
        virtual const std::string& get_name() const = 0;
        virtual const std::string& get_tooltip() const = 0;
        virtual void set_tooltip(const std::string& tooltip) = 0;
        virtual bool is_advanced() const = 0;
        virtual void set_advanced(bool advanced) = 0;
        
        // Callbacks
        virtual void set_on_change_callback(ValueChangedCallback callback) = 0;
        virtual void set_on_apply_callback(ValueAppliedCallback callback) = 0;
        virtual void set_on_external_update_callback(ExternalUpdateCallback callback) = 0;
        
        // External update support
        virtual void update_from_external(bool silent = false) = 0;
        virtual bool is_externally_controlled() const = 0;
        virtual void set_externally_controlled(bool controlled) = 0;
        
        // Edit mode
        virtual EditMode get_edit_mode() const = 0;
        virtual void set_edit_mode(EditMode mode) = 0;
        
        // Value source tracking
        virtual ValueSource get_last_value_source() const = 0;
    };

    // Base template class for ImGui value types
    template <typename T>
    class ImGuiValue : public IImGuiValue
    {
    public:
        using Ptr = std::unique_ptr<ImGuiValue<T>>;

        ImGuiValue(T default_value, const std::string& name = "")
            : m_value(default_value)
            , m_default_value(default_value)
            , m_pending_value(default_value)
            , m_external_value(default_value)
            , m_name(name)
            , m_tooltip("")
            , m_is_advanced(false)
            , m_is_dirty(false)
            , m_externally_controlled(false)
            , m_edit_mode(EditMode::Editable)
            , m_last_value_source(ValueSource::Default)
        {
        }

        ImGuiValue(T default_value, const StringType& name)
            : m_value(default_value)
            , m_default_value(default_value)
            , m_pending_value(default_value)
            , m_external_value(default_value)
            , m_name(to_string(name))
            , m_tooltip("")
            , m_is_advanced(false)
            , m_is_dirty(false)
            , m_externally_controlled(false)
            , m_edit_mode(EditMode::Editable)
            , m_last_value_source(ValueSource::Default)
        {
        }

        virtual ~ImGuiValue() = default;

        // Value access
        T& value() { return m_value; }
        const T& value() const { return m_value; }
        operator T&() { return m_value; }
        operator const T&() const { return m_value; }
        
        // Pending value access (for deferred updates)
        T& pending_value() { return m_pending_value; }
        const T& pending_value() const { return m_pending_value; }
        
        // External value access (for tracking game engine updates)
        T& external_value() { return m_external_value; }
        const T& external_value() const { return m_external_value; }
        
        // Default value access
        const T& default_value() const { return m_default_value; }
        
        // Reset to default
        void reset_to_default() 
        { 
            m_pending_value = m_default_value;
            m_is_dirty = true;
            m_last_value_source = ValueSource::User;
            fire_change_callback();
        }
        
        // Update value from external source (e.g., game engine)
        void set_external_value(const T& new_value)
        {
            m_external_value = new_value;
            
            // Only update the actual value if we're not dirty (no pending user changes)
            if (!m_is_dirty && m_externally_controlled)
            {
                m_value = new_value;
                m_pending_value = new_value;
                m_last_value_source = ValueSource::External;
                
                if (m_on_external_update)
                {
                    m_on_external_update();
                }
            }
        }
        
        // Container support
        bool has_pending_changes() const override { return m_is_dirty; }
        
        void apply_changes() override
        {
            if (m_is_dirty)
            {
                m_value = m_pending_value;
                m_is_dirty = false;
                m_last_value_source = ValueSource::User;
                fire_apply_callback();
            }
        }
        
        void revert_changes() override
        {
            // When reverting, check if we should revert to external value
            if (m_externally_controlled && m_external_value != m_value)
            {
                m_value = m_external_value;
                m_pending_value = m_external_value;
                m_last_value_source = ValueSource::External;
            }
            else
            {
                m_pending_value = m_value;
            }
            m_is_dirty = false;
        }
        
        void mark_dirty() override { m_is_dirty = true; }
        
        // Metadata
        const std::string& get_name() const override { return m_name; }
        const std::string& get_tooltip() const override { return m_tooltip; }
        void set_tooltip(const std::string& tooltip) override { m_tooltip = tooltip; }
        bool is_advanced() const override { return m_is_advanced; }
        void set_advanced(bool advanced) override { m_is_advanced = advanced; }
        
        // Callbacks
        void set_on_change_callback(ValueChangedCallback callback) override { m_on_change = callback; }
        void set_on_apply_callback(ValueAppliedCallback callback) override { m_on_apply = callback; }
        void set_on_external_update_callback(ExternalUpdateCallback callback) override { m_on_external_update = callback; }
        
        // External update support
        void update_from_external(bool silent = false) override
        {
            if (m_externally_controlled && !m_is_dirty)
            {
                m_value = m_external_value;
                m_pending_value = m_external_value;
                m_last_value_source = ValueSource::External;
                
                if (!silent && m_on_external_update)
                {
                    m_on_external_update();
                }
            }
        }
        
        bool is_externally_controlled() const override { return m_externally_controlled; }
        void set_externally_controlled(bool controlled) override { m_externally_controlled = controlled; }
        
        // Edit mode
        EditMode get_edit_mode() const override { return m_edit_mode; }
        void set_edit_mode(EditMode mode) override { m_edit_mode = mode; }
        
        // Value source tracking
        ValueSource get_last_value_source() const override { return m_last_value_source; }
        
        // String conversion
        std::string get_as_string() const override
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                return m_value;
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                return m_value ? "true" : "false";
            }
            else if constexpr (std::is_arithmetic_v<T>)
            {
                return std::to_string(m_value);
            }
            else
            {
                static_assert(std::is_same_v<T, void>, "Unsupported type for get_as_string");
                return "";
            }
        }
        
        void set_from_string(const std::string& value) override
        {
            if constexpr (std::is_same_v<T, std::string>)
            {
                set_value_internal(value);
            }
            else if constexpr (std::is_same_v<T, bool>)
            {
                set_value_internal(value == "true" || value == "1");
            }
            else if constexpr (std::is_integral_v<T>)
            {
                if constexpr (std::is_unsigned_v<T>)
                {
                    set_value_internal(static_cast<T>(std::stoul(value)));
                }
                else
                {
                    set_value_internal(static_cast<T>(std::stol(value)));
                }
            }
            else if constexpr (std::is_floating_point_v<T>)
            {
                set_value_internal(static_cast<T>(std::stod(value)));
            }
            else
            {
                static_assert(std::is_same_v<T, void>, "Unsupported type for set_from_string");
            }
        }

    protected:
        // Helper for setting value with proper callback handling
        void set_value_internal(const T& new_value)
        {
            m_pending_value = new_value;
            m_is_dirty = true;
            m_last_value_source = ValueSource::User;
            fire_change_callback();
        }
        
        // Get the current working value (always pending until applied)
        T& get_working_value()
        {
            return m_pending_value;
        }
        
        const T& get_working_value() const
        {
            return m_pending_value;
        }
        
        // Helper for context menu
        void render_context_menu()
        {
            if (ImGui::BeginPopupContextItem())
            {
                if (ImGui::Button("Reset to default"))
                {
                    reset_to_default();
                }
                
                if (m_is_dirty)
                {
                    ImGui::Separator();
                    if (ImGui::Button("Apply changes"))
                    {
                        apply_changes();
                    }
                    if (ImGui::Button("Revert changes"))
                    {
                        revert_changes();
                    }
                }
                
                ImGui::EndPopup();
            }
        }

        // Get display label - returns provided label or stored name
        const char* get_display_label(const char* override_label = nullptr) const
        {
            if (override_label && override_label[0] != '\0')
            {
                return override_label;
            }
            else if (!m_name.empty())
            {
                return m_name.c_str();
            }
            else
            {
                return "##unnamed"; // Hidden label for ImGui ID
            }
        }
        
        // Fire callbacks
        void fire_change_callback()
        {
            if (m_on_change)
            {
                m_on_change();
            }
        }
        
        void fire_apply_callback()
        {
            if (m_on_apply)
            {
                m_on_apply();
            }
        }

        // Helper to render tooltip if available
        void render_tooltip() const
        {
            if (!m_tooltip.empty() && ImGui::IsItemHovered())
            {
                ImGui::SetTooltip("%s", m_tooltip.c_str());
            }
        }

        T m_value;
        T m_default_value;
        T m_pending_value;
        T m_external_value;
        std::string m_name;
        std::string m_tooltip;
        bool m_is_advanced;
        bool m_is_dirty;
        bool m_externally_controlled;
        EditMode m_edit_mode;
        ValueSource m_last_value_source;
        ValueChangedCallback m_on_change;
        ValueAppliedCallback m_on_apply;
        ExternalUpdateCallback m_on_external_update;
    };

    // Boolean toggle
    class ImGuiToggle : public ImGuiValue<bool>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiToggle>;

        static auto create(bool default_value = false, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiToggle>(default_value, name, tooltip);
        }

        static auto create(bool default_value, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiToggle>(default_value, name, tooltip);
        }

        ImGuiToggle(bool default_value = false, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<bool>(default_value, name)
        {
            m_tooltip = tooltip;
        }

        ImGuiToggle(bool default_value, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<bool>(default_value, name)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label) override
        {
            // Handle different edit modes
            if (m_edit_mode == EditMode::ViewOnly)
            {
                draw_value(label);
                return false;
            }
            else if (m_edit_mode == EditMode::ReadOnly)
            {
                ImGui::PushID(this);
                bool display_val = get_working_value();
                ImGui::BeginDisabled();
                ImGui::Checkbox(get_display_label(label), &display_val);
                ImGui::EndDisabled();
                render_tooltip();
                ImGui::PopID();
                return false;
            }
            
            // Normal editable mode
            ImGui::PushID(this);
            bool& working_val = get_working_value();
            bool changed = ImGui::Checkbox(get_display_label(label), &working_val);
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                m_last_value_source = ValueSource::User;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label) override
        {
            ImGui::Text("%s: %s", get_display_label(label), m_value ? "true" : "false");
        }

        void draw_value(const CharType* label) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        bool toggle() { return m_value = !m_value; }
    };

    // Float input
    class ImGuiFloat : public ImGuiValue<float>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiFloat>;

        static auto create(float default_value = 0.0f, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiFloat>(default_value, name, tooltip);
        }

        static auto create(float default_value, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiFloat>(default_value, name, tooltip);
        }

        ImGuiFloat(float default_value = 0.0f, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<float>(default_value, name)
        {
            m_tooltip = tooltip;
        }

        ImGuiFloat(float default_value, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<float>(default_value, name)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            // Handle different edit modes
            if (m_edit_mode == EditMode::ViewOnly)
            {
                draw_value(label);
                return false;
            }
            else if (m_edit_mode == EditMode::ReadOnly)
            {
                ImGui::PushID(this);
                float display_val = get_working_value();
                ImGui::BeginDisabled();
                ImGui::InputFloat(get_display_label(label), &display_val);
                ImGui::EndDisabled();
                render_tooltip();
                ImGui::PopID();
                return false;
            }
            
            // Normal editable mode
            ImGui::PushID(this);
            float& working_val = get_working_value();
            bool changed = ImGui::InputFloat(get_display_label(label), &working_val);
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                m_last_value_source = ValueSource::User;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %.3f", get_display_label(label), m_value);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }
    };

    // Double input
    class ImGuiDouble : public ImGuiValue<double>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiDouble>;

        static auto create(double default_value = 0.0, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiDouble>(default_value, name, tooltip);
        }

        static auto create(double default_value, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiDouble>(default_value, name, tooltip);
        }

        ImGuiDouble(double default_value = 0.0, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<double>(default_value, name)
        {
            m_tooltip = tooltip;
        }

        ImGuiDouble(double default_value, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<double>(default_value, name)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            // Handle different edit modes
            if (m_edit_mode == EditMode::ViewOnly)
            {
                draw_value(label);
                return false;
            }
            else if (m_edit_mode == EditMode::ReadOnly)
            {
                ImGui::PushID(this);
                double display_val = get_working_value();
                ImGui::BeginDisabled();
                ImGui::InputDouble(get_display_label(label), &display_val);
                ImGui::EndDisabled();
                render_tooltip();
                ImGui::PopID();
                return false;
            }
            
            // Normal editable mode
            ImGui::PushID(this);
            double& working_val = get_working_value();
            bool changed = ImGui::InputDouble(get_display_label(label), &working_val);
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                m_last_value_source = ValueSource::User;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %.6f", get_display_label(label), m_value);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }
    };

    // Double slider (uses float slider internally but stores as double)
    class ImGuiSliderDouble : public ImGuiValue<double>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiSliderDouble>;

        static auto create(double min = 0.0, double max = 1.0, double default_value = 0.0, const std::string& name = "", const std::string& tooltip = "", bool show_precision_input = false)
        {
            return std::make_unique<ImGuiSliderDouble>(min, max, default_value, name, tooltip, show_precision_input);
        }

        static auto create(double min, double max, double default_value, const StringType& name, const StringType& tooltip = STR(""), bool show_precision_input = false)
        {
            return std::make_unique<ImGuiSliderDouble>(min, max, default_value, name, tooltip, show_precision_input);
        }

        ImGuiSliderDouble(double min = 0.0, double max = 1.0, double default_value = 0.0, const std::string& name = "", const std::string& tooltip = "", bool show_precision_input = false)
            : ImGuiValue<double>(default_value, name)
            , m_min(min)
            , m_max(max)
            , m_show_precision_input(show_precision_input)
        {
            m_tooltip = tooltip;
        }

        ImGuiSliderDouble(double min, double max, double default_value, const StringType& name, const StringType& tooltip = STR(""), bool show_precision_input = false)
            : ImGuiValue<double>(default_value, name)
            , m_min(min)
            , m_max(max)
            , m_show_precision_input(show_precision_input)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            // Handle different edit modes
            if (m_edit_mode == EditMode::ViewOnly)
            {
                draw_value(label);
                return false;
            }
            else if (m_edit_mode == EditMode::ReadOnly)
            {
                ImGui::PushID(this);
                float display_val = static_cast<float>(get_working_value());
                ImGui::BeginDisabled();
                ImGui::SliderFloat(get_display_label(label), &display_val, static_cast<float>(m_min), static_cast<float>(m_max));
                if (m_show_precision_input)
                {
                    ImGui::SameLine();
                    double precise_val = get_working_value();
                    ImGui::InputDouble("##precise", &precise_val);
                }
                ImGui::EndDisabled();
                render_tooltip();
                ImGui::PopID();
                return false;
            }
            
            // Normal editable mode - use float slider internally
            ImGui::PushID(this);
            bool changed = false;
            double& working_val = get_working_value();
            
            // Slider
            float float_val = static_cast<float>(working_val);
            if (ImGui::SliderFloat(get_display_label(label), &float_val, 
                                  static_cast<float>(m_min), static_cast<float>(m_max)))
            {
                working_val = static_cast<double>(float_val);
                changed = true;
            }
            
            // Optional precision input
            if (m_show_precision_input)
            {
                ImGui::SameLine();
                if (ImGui::InputDouble("##precise", &working_val))
                {
                    working_val = std::clamp(working_val, m_min, m_max);
                    changed = true;
                }
            }
            
            if (changed)
            {
                m_is_dirty = true;
                m_last_value_source = ValueSource::User;
                fire_change_callback();
            }
            
            render_tooltip();
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %.6f [%.6f, %.6f]", get_display_label(label), m_value, m_min, m_max);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        double& min() { return m_min; }
        double& max() { return m_max; }
        void show_precision_input(bool show) { m_show_precision_input = show; }

    private:
        double m_min;
        double m_max;
        bool m_show_precision_input;
    };

    // Float slider
    class ImGuiSlider : public ImGuiValue<float>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiSlider>;

        static auto create(float min = 0.0f, float max = 1.0f, float default_value = 0.0f, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiSlider>(min, max, default_value, name, tooltip);
        }

        static auto create(float min, float max, float default_value, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiSlider>(min, max, default_value, name, tooltip);
        }

        ImGuiSlider(float min = 0.0f, float max = 1.0f, float default_value = 0.0f, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<float>(default_value, name)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = tooltip;
        }

        ImGuiSlider(float min, float max, float default_value, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<float>(default_value, name)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            float& working_val = get_working_value();
            bool changed = ImGui::SliderFloat(get_display_label(label), &working_val, m_min, m_max);
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %.3f [%.3f, %.3f]", get_display_label(label), m_value, m_min, m_max);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        float& min() { return m_min; }
        float& max() { return m_max; }

    private:
        float m_min;
        float m_max;
    };

    // Integer input
    class ImGuiInt32 : public ImGuiValue<int32_t>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiInt32>;

        static auto create(int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiInt32>(default_value, name, tooltip);
        }

        static auto create(int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiInt32>(default_value, name, tooltip);
        }

        ImGuiInt32(int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<int32_t>(default_value, name)
        {
            m_tooltip = tooltip;
        }

        ImGuiInt32(int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<int32_t>(default_value, name)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            int32_t& working_val = get_working_value();
            bool changed = ImGui::InputInt(get_display_label(label), &working_val);
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %d", get_display_label(label), m_value);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }
    };

    // Integer slider
    class ImGuiSliderInt32 : public ImGuiValue<int32_t>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiSliderInt32>;

        static auto create(int32_t min = 0, int32_t max = 100, int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiSliderInt32>(min, max, default_value, name, tooltip);
        }

        static auto create(int32_t min, int32_t max, int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiSliderInt32>(min, max, default_value, name, tooltip);
        }

        ImGuiSliderInt32(int32_t min = 0, int32_t max = 100, int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<int32_t>(default_value, name)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = tooltip;
        }

        ImGuiSliderInt32(int32_t min, int32_t max, int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<int32_t>(default_value, name)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            int32_t& working_val = get_working_value();
            bool changed = ImGui::SliderInt(get_display_label(label), &working_val, m_min, m_max);
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %d [%d, %d]", get_display_label(label), m_value, m_min, m_max);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        int32_t& min() { return m_min; }
        int32_t& max() { return m_max; }

    private:
        int32_t m_min;
        int32_t m_max;
    };

    // Combo box
    class ImGuiCombo : public ImGuiValue<int32_t>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiCombo>;

        static auto create(const std::vector<std::string>& options, int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiCombo>(options, default_value, name, tooltip);
        }

        static auto create(const std::vector<std::string>& options, int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiCombo>(options, default_value, name, tooltip);
        }

        ImGuiCombo(const std::vector<std::string>& options, int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<int32_t>(default_value, name)
            , m_options_strings(options)
        {
            m_tooltip = tooltip;
            update_options_pointers();
        }

        ImGuiCombo(const std::vector<std::string>& options, int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<int32_t>(default_value, name)
            , m_options_strings(options)
        {
            m_tooltip = to_string(tooltip);
            update_options_pointers();
        }

        bool draw(const char* label = nullptr) override
        {
            // Clamp value to valid range
            int32_t& working_val = get_working_value();
            working_val = std::clamp<int32_t>(working_val, 0, static_cast<int32_t>(m_options_pointers.size()) - 1);

            ImGui::PushID(this);
            bool changed = ImGui::Combo(get_display_label(label), &working_val, m_options_pointers.data(), static_cast<int32_t>(m_options_pointers.size()));
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            m_value = std::clamp<int32_t>(m_value, 0, static_cast<int32_t>(m_options_pointers.size()) - 1);
            ImGui::Text("%s: %s", get_display_label(label), m_options_pointers[m_value]);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        const std::vector<std::string>& options() const { return m_options_strings; }
        
        void set_options(const std::vector<std::string>& options)
        {
            m_options_strings = options;
            update_options_pointers();
        }

    private:
        void update_options_pointers()
        {
            m_options_pointers.clear();
            for (const auto& option : m_options_strings)
            {
                m_options_pointers.push_back(option.c_str());
            }
        }

        std::vector<std::string> m_options_strings;
        std::vector<const char*> m_options_pointers;
    };

    // String input
    class ImGuiString : public ImGuiValue<std::string>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiString>;

        static auto create(const std::string& default_value = "", const std::string& name = "", const std::string& tooltip = "", size_t buffer_size = 256)
        {
            return std::make_unique<ImGuiString>(default_value, name, tooltip, buffer_size);
        }

        static auto create(const std::string& default_value, const StringType& name, const StringType& tooltip = STR(""), size_t buffer_size = 256)
        {
            return std::make_unique<ImGuiString>(default_value, name, tooltip, buffer_size);
        }

        ImGuiString(const std::string& default_value = "", const std::string& name = "", const std::string& tooltip = "", size_t buffer_size = 256)
            : ImGuiValue<std::string>(default_value, name)
            , m_buffer_size(buffer_size)
        {
            m_tooltip = tooltip;
            m_buffer.resize(m_buffer_size);
            std::copy(m_value.begin(), m_value.end(), m_buffer.begin());
        }

        ImGuiString(const std::string& default_value, const StringType& name, const StringType& tooltip = STR(""), size_t buffer_size = 256)
            : ImGuiValue<std::string>(default_value, name)
            , m_buffer_size(buffer_size)
        {
            m_tooltip = to_string(tooltip);
            m_buffer.resize(m_buffer_size);
            std::copy(m_value.begin(), m_value.end(), m_buffer.begin());
        }

        bool draw(const char* label = nullptr) override
        {
            // Update buffer with working value
            const std::string& working_val = get_working_value();
            std::fill(m_buffer.begin(), m_buffer.end(), 0);
            std::copy(working_val.begin(), working_val.end(), m_buffer.begin());
            
            ImGui::PushID(this);
            bool changed = ImGui::InputText(get_display_label(label), m_buffer.data(), m_buffer_size);
            render_tooltip();
            if (changed)
            {
                std::string new_value = std::string(m_buffer.data());
                m_pending_value = new_value;
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %s", get_display_label(label), m_value.c_str());
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

    private:
        size_t m_buffer_size;
        std::vector<char> m_buffer;
    };

    // Multi-line string input
    class ImGuiTextMultiline : public ImGuiValue<std::string>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiTextMultiline>;

        static auto create(const std::string& default_value = "", const std::string& name = "", const std::string& tooltip = "", ImVec2 size = ImVec2(0, 0), size_t buffer_size = 4096)
        {
            return std::make_unique<ImGuiTextMultiline>(default_value, name, tooltip, size, buffer_size);
        }

        static auto create(const std::string& default_value, const StringType& name, const StringType& tooltip = STR(""), ImVec2 size = ImVec2(0, 0), size_t buffer_size = 4096)
        {
            return std::make_unique<ImGuiTextMultiline>(default_value, name, tooltip, size, buffer_size);
        }

        ImGuiTextMultiline(const std::string& default_value = "", const std::string& name = "", const std::string& tooltip = "", ImVec2 size = ImVec2(0, 0), size_t buffer_size = 4096)
            : ImGuiValue<std::string>(default_value, name)
            , m_size(size)
            , m_buffer_size(buffer_size)
        {
            m_tooltip = tooltip;
            m_buffer.resize(m_buffer_size);
            std::copy(m_value.begin(), m_value.end(), m_buffer.begin());
        }

        ImGuiTextMultiline(const std::string& default_value, const StringType& name, const StringType& tooltip = STR(""), ImVec2 size = ImVec2(0, 0), size_t buffer_size = 4096)
            : ImGuiValue<std::string>(default_value, name)
            , m_size(size)
            , m_buffer_size(buffer_size)
        {
            m_tooltip = to_string(tooltip);
            m_buffer.resize(m_buffer_size);
            std::copy(m_value.begin(), m_value.end(), m_buffer.begin());
        }

        bool draw(const char* label = nullptr) override
        {
            // Update buffer with working value
            const std::string& working_val = get_working_value();
            std::fill(m_buffer.begin(), m_buffer.end(), 0);
            std::copy(working_val.begin(), working_val.end(), m_buffer.begin());
            
            ImGui::PushID(this);
            bool changed = ImGui::InputTextMultiline(get_display_label(label), m_buffer.data(), m_buffer_size, m_size);
            render_tooltip();
            if (changed)
            {
                std::string new_value = std::string(m_buffer.data());
                m_pending_value = new_value;
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %s", get_display_label(label), m_value.c_str());
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        ImVec2& size() { return m_size; }

    private:
        ImVec2 m_size;
        size_t m_buffer_size;
        std::vector<char> m_buffer;
    };

    // Color picker (RGB)
    class ImGuiColor3 : public ImGuiValue<std::array<float, 3>>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiColor3>;

        static auto create(float r = 1.0f, float g = 1.0f, float b = 1.0f, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiColor3>(r, g, b, name, tooltip);
        }

        static auto create(float r, float g, float b, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiColor3>(r, g, b, name, tooltip);
        }

        ImGuiColor3(float r = 1.0f, float g = 1.0f, float b = 1.0f, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<std::array<float, 3>>({r, g, b}, name)
        {
            m_tooltip = tooltip;
        }

        ImGuiColor3(float r, float g, float b, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<std::array<float, 3>>({r, g, b}, name)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            std::array<float, 3>& working_val = get_working_value();
            bool changed = ImGui::ColorEdit3(get_display_label(label), working_val.data());
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: (%.3f, %.3f, %.3f)", get_display_label(label), m_value[0], m_value[1], m_value[2]);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        float& r() { return m_value[0]; }
        float& g() { return m_value[1]; }
        float& b() { return m_value[2]; }
    };

    // Color picker (RGBA)
    class ImGuiColor4 : public ImGuiValue<std::array<float, 4>>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiColor4>;

        static auto create(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiColor4>(r, g, b, a, name, tooltip);
        }

        static auto create(float r, float g, float b, float a, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiColor4>(r, g, b, a, name, tooltip);
        }

        ImGuiColor4(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<std::array<float, 4>>({r, g, b, a}, name)
        {
            m_tooltip = tooltip;
        }

        ImGuiColor4(float r, float g, float b, float a, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<std::array<float, 4>>({r, g, b, a}, name)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            std::array<float, 4>& working_val = get_working_value();
            bool changed = ImGui::ColorEdit4(get_display_label(label), working_val.data());
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: (%.3f, %.3f, %.3f, %.3f)", get_display_label(label), m_value[0], m_value[1], m_value[2], m_value[3]);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        float& r() { return m_value[0]; }
        float& g() { return m_value[1]; }
        float& b() { return m_value[2]; }
        float& a() { return m_value[3]; }
    };

    // Vector2 input
    class ImGuiVector2 : public ImGuiValue<std::array<float, 2>>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiVector2>;

        static auto create(float x = 0.0f, float y = 0.0f, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiVector2>(x, y, name, tooltip);
        }

        static auto create(float x, float y, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiVector2>(x, y, name, tooltip);
        }

        ImGuiVector2(float x = 0.0f, float y = 0.0f, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<std::array<float, 2>>({x, y}, name)
        {
            m_tooltip = tooltip;
        }

        ImGuiVector2(float x, float y, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<std::array<float, 2>>({x, y}, name)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            std::array<float, 2>& working_val = get_working_value();
            bool changed = ImGui::InputFloat2(get_display_label(label), working_val.data());
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: (%.3f, %.3f)", get_display_label(label), m_value[0], m_value[1]);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        float& x() { return m_value[0]; }
        float& y() { return m_value[1]; }
    };

    // Vector3 input
    class ImGuiVector3 : public ImGuiValue<std::array<float, 3>>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiVector3>;

        static auto create(float x = 0.0f, float y = 0.0f, float z = 0.0f, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiVector3>(x, y, z, name, tooltip);
        }

        static auto create(float x, float y, float z, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiVector3>(x, y, z, name, tooltip);
        }

        ImGuiVector3(float x = 0.0f, float y = 0.0f, float z = 0.0f, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<std::array<float, 3>>({x, y, z}, name)
        {
            m_tooltip = tooltip;
        }

        ImGuiVector3(float x, float y, float z, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<std::array<float, 3>>({x, y, z}, name)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            std::array<float, 3>& working_val = get_working_value();
            bool changed = ImGui::InputFloat3(get_display_label(label), working_val.data());
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: (%.3f, %.3f, %.3f)", get_display_label(label), m_value[0], m_value[1], m_value[2]);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        float& x() { return m_value[0]; }
        float& y() { return m_value[1]; }
        float& z() { return m_value[2]; }
    };

    // Drag float
    class ImGuiDragFloat : public ImGuiValue<float>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiDragFloat>;

        static auto create(float default_value = 0.0f, const std::string& name = "", const std::string& tooltip = "", float speed = 1.0f, float min = 0.0f, float max = 0.0f)
        {
            return std::make_unique<ImGuiDragFloat>(default_value, name, tooltip, speed, min, max);
        }

        static auto create(float default_value, const StringType& name, const StringType& tooltip = STR(""), float speed = 1.0f, float min = 0.0f, float max = 0.0f)
        {
            return std::make_unique<ImGuiDragFloat>(default_value, name, tooltip, speed, min, max);
        }

        ImGuiDragFloat(float default_value = 0.0f, const std::string& name = "", const std::string& tooltip = "", float speed = 1.0f, float min = 0.0f, float max = 0.0f)
            : ImGuiValue<float>(default_value, name)
            , m_speed(speed)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = tooltip;
        }

        ImGuiDragFloat(float default_value, const StringType& name, const StringType& tooltip = STR(""), float speed = 1.0f, float min = 0.0f, float max = 0.0f)
            : ImGuiValue<float>(default_value, name)
            , m_speed(speed)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            float& working_val = get_working_value();
            bool changed = ImGui::DragFloat(get_display_label(label), &working_val, m_speed, m_min, m_max);
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %.3f", get_display_label(label), m_value);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        float& speed() { return m_speed; }
        float& min() { return m_min; }
        float& max() { return m_max; }

    private:
        float m_speed;
        float m_min;
        float m_max;
    };

    // Drag double (uses float drag internally but stores as double)
    class ImGuiDragDouble : public ImGuiValue<double>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiDragDouble>;

        static auto create(double default_value = 0.0, const std::string& name = "", const std::string& tooltip = "", float speed = 1.0f, double min = 0.0, double max = 0.0)
        {
            return std::make_unique<ImGuiDragDouble>(default_value, name, tooltip, speed, min, max);
        }

        static auto create(double default_value, const StringType& name, const StringType& tooltip = STR(""), float speed = 1.0f, double min = 0.0, double max = 0.0)
        {
            return std::make_unique<ImGuiDragDouble>(default_value, name, tooltip, speed, min, max);
        }

        ImGuiDragDouble(double default_value = 0.0, const std::string& name = "", const std::string& tooltip = "", float speed = 1.0f, double min = 0.0, double max = 0.0)
            : ImGuiValue<double>(default_value, name)
            , m_speed(speed)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = tooltip;
        }

        ImGuiDragDouble(double default_value, const StringType& name, const StringType& tooltip = STR(""), float speed = 1.0f, double min = 0.0, double max = 0.0)
            : ImGuiValue<double>(default_value, name)
            , m_speed(speed)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            // Handle different edit modes
            if (m_edit_mode == EditMode::ViewOnly)
            {
                draw_value(label);
                return false;
            }
            else if (m_edit_mode == EditMode::ReadOnly)
            {
                ImGui::PushID(this);
                float display_val = static_cast<float>(get_working_value());
                ImGui::BeginDisabled();
                ImGui::DragFloat(get_display_label(label), &display_val, m_speed, static_cast<float>(m_min), static_cast<float>(m_max));
                ImGui::EndDisabled();
                render_tooltip();
                ImGui::PopID();
                return false;
            }
            
            // Normal editable mode - use float drag internally
            ImGui::PushID(this);
            double& working_val = get_working_value();
            float float_val = static_cast<float>(working_val);
            bool changed = ImGui::DragFloat(get_display_label(label), &float_val, m_speed, 
                                           static_cast<float>(m_min), static_cast<float>(m_max));
            if (changed)
            {
                working_val = static_cast<double>(float_val);
                m_is_dirty = true;
                m_last_value_source = ValueSource::User;
                fire_change_callback();
            }
            render_tooltip();
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %.6f", get_display_label(label), m_value);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        float& speed() { return m_speed; }
        double& min() { return m_min; }
        double& max() { return m_max; }

    private:
        float m_speed;
        double m_min;
        double m_max;
    };

    // Drag int
    class ImGuiDragInt : public ImGuiValue<int32_t>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiDragInt>;

        static auto create(int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "", float speed = 1.0f, int32_t min = 0, int32_t max = 0)
        {
            return std::make_unique<ImGuiDragInt>(default_value, name, tooltip, speed, min, max);
        }

        static auto create(int32_t default_value, const StringType& name, const StringType& tooltip = STR(""), float speed = 1.0f, int32_t min = 0, int32_t max = 0)
        {
            return std::make_unique<ImGuiDragInt>(default_value, name, tooltip, speed, min, max);
        }

        ImGuiDragInt(int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "", float speed = 1.0f, int32_t min = 0, int32_t max = 0)
            : ImGuiValue<int32_t>(default_value, name)
            , m_speed(speed)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = tooltip;
        }

        ImGuiDragInt(int32_t default_value, const StringType& name, const StringType& tooltip = STR(""), float speed = 1.0f, int32_t min = 0, int32_t max = 0)
            : ImGuiValue<int32_t>(default_value, name)
            , m_speed(speed)
            , m_min(min)
            , m_max(max)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            int32_t& working_val = get_working_value();
            bool changed = ImGui::DragInt(get_display_label(label), &working_val, m_speed, m_min, m_max);
            render_tooltip();
            if (changed)
            {
                m_is_dirty = true;
                fire_change_callback();
            }
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: %d", get_display_label(label), m_value);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        float& speed() { return m_speed; }
        int32_t& min() { return m_min; }
        int32_t& max() { return m_max; }

    private:
        float m_speed;
        int32_t m_min;
        int32_t m_max;
    };

    // Radio button group
    class ImGuiRadioButton : public ImGuiValue<int32_t>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiRadioButton>;

        static auto create(const std::vector<std::string>& options, int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiRadioButton>(options, default_value, name, tooltip);
        }

        static auto create(const std::vector<std::string>& options, int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiRadioButton>(options, default_value, name, tooltip);
        }

        ImGuiRadioButton(const std::vector<std::string>& options, int32_t default_value = 0, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<int32_t>(default_value, name)
            , m_options(options)
        {
            m_tooltip = tooltip;
        }

        ImGuiRadioButton(const std::vector<std::string>& options, int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<int32_t>(default_value, name)
            , m_options(options)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            ImGui::Text("%s", get_display_label(label));
            render_tooltip();
            
            bool changed = false;
            int32_t& working_val = get_working_value();
            for (size_t i = 0; i < m_options.size(); ++i)
            {
                if (ImGui::RadioButton(m_options[i].c_str(), &working_val, static_cast<int32_t>(i)))
                {
                    changed = true;
                    m_is_dirty = true;
                    fire_change_callback();
                }
                if (i < m_options.size() - 1)
                {
                    ImGui::SameLine();
                }
            }
            
            render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            if (m_value >= 0 && m_value < static_cast<int32_t>(m_options.size()))
            {
                ImGui::Text("%s: %s", get_display_label(label), m_options[m_value].c_str());
            }
            else
            {
                ImGui::Text("%s: Invalid", get_display_label(label));
            }
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        const std::vector<std::string>& options() const { return m_options; }

    private:
        std::vector<std::string> m_options;
    };

    // Key binding selector
    class ImGuiKey : public ImGuiValue<int32_t>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiKey>;

        static constexpr int32_t UNBOUND_KEY = -1;

        static auto create(int32_t default_value = UNBOUND_KEY, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiKey>(default_value, name, tooltip);
        }

        static auto create(int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
        {
            return std::make_unique<ImGuiKey>(default_value, name, tooltip);
        }

        ImGuiKey(int32_t default_value = UNBOUND_KEY, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<int32_t>(default_value, name)
        {
            m_tooltip = tooltip;
        }

        ImGuiKey(int32_t default_value, const StringType& name, const StringType& tooltip = STR(""))
            : ImGuiValue<int32_t>(default_value, name)
        {
            m_tooltip = to_string(tooltip);
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            
            if (ImGui::Button(get_display_label(label)))
            {
                m_waiting_for_key = true;
            }
            render_tooltip();
            
            ImGui::SameLine();
            
            bool changed = false;
            int32_t& working_val = get_working_value();
            
            if (m_waiting_for_key)
            {
                ImGui::Text("Press any key...");
                
                // Check for key press
                for (int key = ImGuiKey_NamedKey_BEGIN; key < ImGuiKey_NamedKey_END; ++key)
                {
                    if (ImGui::IsKeyPressed(static_cast<ImGuiKey>(key)))
                    {
                        // Escape or Backspace clears the binding
                        if (key == ImGuiKey_Escape || key == ImGuiKey_Backspace)
                        {
                            working_val = UNBOUND_KEY;
                        }
                        else
                        {
                            working_val = key;
                        }
                        m_waiting_for_key = false;
                        changed = true;
                        
                        m_is_dirty = true;
                        fire_change_callback();
                        break;
                    }
                }
            }
            else
            {
                if (working_val != UNBOUND_KEY)
                {
                    ImGui::Text("%s", ImGui::GetKeyName(static_cast<ImGuiKey>(working_val)));
                }
                else
                {
                    ImGui::Text("Not bound");
                }
            }
            
            render_context_menu();
            ImGui::PopID();
            
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            if (m_value != UNBOUND_KEY)
            {
                ImGui::Text("%s: %s", get_display_label(label), ImGui::GetKeyName(static_cast<ImGuiKey>(m_value)));
            }
            else
            {
                ImGui::Text("%s: Not bound", get_display_label(label));
            }
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        bool is_key_down() const
        {
            if (m_value == UNBOUND_KEY) return false;
            return ImGui::IsKeyDown(static_cast<ImGuiKey>(m_value));
        }

        bool is_key_pressed() const
        {
            if (m_value == UNBOUND_KEY) return false;
            return ImGui::IsKeyPressed(static_cast<ImGuiKey>(m_value));
        }

    private:
        bool m_waiting_for_key = false;
    };

    // Flags editor (checkboxes for bit flags)
    template<typename FlagType = uint32_t>
    class ImGuiFlags : public ImGuiValue<FlagType>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiFlags<FlagType>>;

        struct FlagInfo
        {
            std::string name;
            FlagType value;
        };

        static auto create(const std::vector<FlagInfo>& flags, FlagType default_value = 0, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiFlags<FlagType>>(flags, default_value, name, tooltip);
        }

        ImGuiFlags(const std::vector<FlagInfo>& flags, FlagType default_value = 0, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<FlagType>(default_value, name)
            , m_flags(flags)
        {
            this->m_tooltip = tooltip;
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            ImGui::Text("%s", this->get_display_label(label));
            this->render_tooltip();
            
            bool changed = false;
            FlagType& working_val = this->get_working_value();
            for (const auto& flag : m_flags)
            {
                bool checked = (working_val & flag.value) != 0;
                if (ImGui::Checkbox(flag.name.c_str(), &checked))
                {
                    if (checked)
                    {
                        working_val |= flag.value;
                    }
                    else
                    {
                        working_val &= ~flag.value;
                    }
                    changed = true;
                }
            }
            
            if (changed)
            {
                this->m_is_dirty = true;
                this->fire_change_callback();
            }
            
            this->render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            ImGui::Text("%s: 0x%X", this->get_display_label(label), this->m_value);
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        const std::vector<FlagInfo>& flags() const { return m_flags; }

    private:
        std::vector<FlagInfo> m_flags;
    };

    // Type-safe enum selector
    template<typename EnumType>
    class ImGuiEnum : public ImGuiValue<EnumType>
    {
    public:
        using Ptr = std::unique_ptr<ImGuiEnum<EnumType>>;

        struct EnumOption
        {
            std::string name;
            EnumType value;
        };

        static auto create(const std::vector<EnumOption>& options, EnumType default_value, const std::string& name = "", const std::string& tooltip = "")
        {
            return std::make_unique<ImGuiEnum<EnumType>>(options, default_value, name, tooltip);
        }

        ImGuiEnum(const std::vector<EnumOption>& options, EnumType default_value, const std::string& name = "", const std::string& tooltip = "")
            : ImGuiValue<EnumType>(default_value, name)
            , m_options(options)
        {
            this->m_tooltip = tooltip;
            update_option_pointers();
        }

        bool draw(const char* label = nullptr) override
        {
            ImGui::PushID(this);
            
            int current_index = find_current_index();
            bool changed = false;
            
            if (ImGui::Combo(this->get_display_label(label), &current_index, m_option_pointers.data(), static_cast<int>(m_option_pointers.size())))
            {
                if (current_index >= 0 && current_index < static_cast<int>(m_options.size()))
                {
                    EnumType& working_val = this->get_working_value();
                    working_val = m_options[current_index].value;
                    changed = true;
                    
                    this->m_is_dirty = true;
                    this->fire_change_callback();
                }
            }
            this->render_tooltip();
            
            this->render_context_menu();
            ImGui::PopID();
            return changed;
        }

        bool draw(const CharType* label = nullptr) override
        {
            return draw(label ? to_string(label).c_str() : nullptr);
        }

        void draw_value(const char* label = nullptr) override
        {
            int index = find_current_index();
            if (index >= 0 && index < static_cast<int>(m_options.size()))
            {
                ImGui::Text("%s: %s", this->get_display_label(label), m_options[index].name.c_str());
            }
            else
            {
                ImGui::Text("%s: Unknown", this->get_display_label(label));
            }
        }

        void draw_value(const CharType* label = nullptr) override
        {
            draw_value(label ? to_string(label).c_str() : nullptr);
        }

        const std::vector<EnumOption>& options() const { return m_options; }

    private:
        int find_current_index() const
        {
            const EnumType& working_val = this->get_working_value();
            for (size_t i = 0; i < m_options.size(); ++i)
            {
                if (m_options[i].value == working_val)
                {
                    return static_cast<int>(i);
                }
            }
            return -1;
        }

        void update_option_pointers()
        {
            m_option_pointers.clear();
            for (const auto& option : m_options)
            {
                m_option_pointers.push_back(option.name.c_str());
            }
        }

        std::vector<EnumOption> m_options;
        std::vector<const char*> m_option_pointers;
    };

    // Template for creating monitored values that track external changes
    template<typename T, typename ValueType>
    class ImGuiMonitoredValue : public ValueType
    {
    public:
        using ValueGetter = std::function<T()>;
        using ValueSetter = std::function<void(const T&)>;
        
        ImGuiMonitoredValue(ValueGetter getter, ValueSetter setter, T default_value, const std::string& name = "")
            : ValueType(default_value, name)
            , m_getter(getter)
            , m_setter(setter)
        {
            this->set_externally_controlled(true);
            this->set_external_value(getter());
        }
        
        // Update from external source
        void refresh()
        {
            if (m_getter)
            {
                T current_external = m_getter();
                if (current_external != this->external_value())
                {
                    this->set_external_value(current_external);
                }
            }
        }
        
        // Override apply to also update external
        void apply_changes_with_external()
        {
            this->apply_changes();
            if (m_setter && this->get_last_value_source() == ValueSource::User)
            {
                m_setter(this->value());
            }
        }
        
    private:
        ValueGetter m_getter;
        ValueSetter m_setter;
    };

    // Helper factory functions for monitored values
    template<typename T>
    auto make_monitored_float(std::function<float()> getter, std::function<void(float)> setter, 
                             float default_value = 0.0f, const std::string& name = "")
    {
        return std::make_unique<ImGuiMonitoredValue<float, ImGuiFloat>>(getter, setter, default_value, name);
    }
    
    template<typename T>
    auto make_monitored_double(std::function<double()> getter, std::function<void(double)> setter, 
                              double default_value = 0.0, const std::string& name = "")
    {
        return std::make_unique<ImGuiMonitoredValue<double, ImGuiDouble>>(getter, setter, default_value, name);
    }
    
    template<typename T>
    auto make_monitored_bool(std::function<bool()> getter, std::function<void(bool)> setter, 
                            bool default_value = false, const std::string& name = "")
    {
        return std::make_unique<ImGuiMonitoredValue<bool, ImGuiToggle>>(getter, setter, default_value, name);
    }

    // Container for managing multiple ImGui values with advanced features
    class ImGuiValueContainer
    {
    public:
        using Ptr = std::unique_ptr<ImGuiValueContainer>;
        using ContainerChangedCallback = std::function<void(const std::string& value_id)>;
        using ContainerAppliedCallback = std::function<void()>;

        ImGuiValueContainer(const std::string& title = "Settings")
            : m_title(title)
            , m_show_advanced(false)
            , m_has_changes(false)
            , m_visible(true)
            , m_unique_id(generate_unique_id())
            , m_global_edit_mode(EditMode::Editable)
            , m_apply_global_edit_mode(false)
            , m_show_edit_mode_control(false)
        {
        }

        ImGuiValueContainer(const StringType& title)
            : m_title(to_string(title))
            , m_show_advanced(false)
            , m_has_changes(false)
            , m_visible(true)
            , m_unique_id(generate_unique_id())
            , m_global_edit_mode(EditMode::Editable)
            , m_apply_global_edit_mode(false)
            , m_show_edit_mode_control(false)
        {
        }

        // Add a value to the container
        template<typename T>
        T* add_value(const std::string& id, std::unique_ptr<T> value)
        {
            static_assert(std::is_base_of_v<IImGuiValue, T>, "T must derive from IImGuiValue");
            T* ptr = value.get();
            
            // Apply global edit mode to new value if enabled
            if (m_apply_global_edit_mode)
            {
                value->set_edit_mode(m_global_edit_mode);
            }
            
            // Wire up container-level change notification
            value->set_on_change_callback([this, id]() {
                if (m_on_value_changed)
                {
                    m_on_value_changed(id);
                }
            });
            
            m_values[id] = std::move(value);
            m_value_order.push_back(id);
            return ptr;
        }

        // Get a value by ID
        template<typename T>
        T* get_value(const std::string& id)
        {
            auto it = m_values.find(id);
            if (it != m_values.end())
            {
                return dynamic_cast<T*>(it->second.get());
            }
            return nullptr;
        }

        // Draw all values
        void draw(bool show_built_in_apply_button = true)
        {
            if (!m_visible) return;
            
            ImGui::PushID(m_unique_id);
            
            if (!m_title.empty())
            {
                ImGui::Text("%s", m_title.c_str());
                ImGui::Separator();
            }

            // Edit mode control checkbox
            if (m_show_edit_mode_control)
            {
                bool is_editable = m_global_edit_mode == EditMode::Editable;
                if (ImGui::Checkbox("Allow Editing", &is_editable))
                {
                    set_global_edit_mode(is_editable ? EditMode::Editable : EditMode::ReadOnly);
                }
                ImGui::Spacing();
            }

            // Advanced toggle
            if (has_advanced_values())
            {
                ImGui::Checkbox("Show Advanced Settings", &m_show_advanced);
                ImGui::Spacing();
            }

            // Draw values in order
            for (const auto& id : m_value_order)
            {
                auto& value = m_values[id];
                
                // Skip advanced values if not showing
                if (value->is_advanced() && !m_show_advanced)
                {
                    continue;
                }

                // Apply global edit mode if set
                if (m_apply_global_edit_mode)
                {
                    value->set_edit_mode(m_global_edit_mode);
                }

                // Visual indicator for pending changes
                if (value->has_pending_changes())
                {
                    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.0f, 1.0f));
                }

                if (value->draw())
                {
                    m_has_changes = true;
                }

                if (value->has_pending_changes())
                {
                    ImGui::PopStyleColor();
                    ImGui::SameLine();
                    ImGui::TextDisabled("*");
                }
            }

            // Apply/Revert buttons for deferred updates
            if (show_built_in_apply_button && has_pending_changes())
            {
                ImGui::Separator();
                ImGui::Spacing();
                
                if (ImGui::Button("Apply All"))
                {
                    apply_all();
                }
                
                ImGui::SameLine();
                
                if (ImGui::Button("Revert All"))
                {
                    revert_all();
                }
            }
            
            ImGui::PopID();
        }
        
        // Draw values without built-in apply button
        void draw_without_apply_button()
        {
            draw(false);
        }
        
        // Draw a custom apply button that's bound to this container
        bool draw_apply_button(const char* label = "Apply", bool show_only_if_changes = true)
        {
            if (show_only_if_changes && !has_pending_changes())
            {
                return false;
            }
            
            if (ImGui::Button(label))
            {
                apply_all();
                return true;
            }
            return false;
        }
        
        // Draw a custom revert button
        bool draw_revert_button(const char* label = "Revert", bool show_only_if_changes = true)
        {
            if (show_only_if_changes && !has_pending_changes())
            {
                return false;
            }
            
            if (ImGui::Button(label))
            {
                revert_all();
                return true;
            }
            return false;
        }

        // Check if any values have pending changes
        bool has_pending_changes() const
        {
            for (const auto& [id, value] : m_values)
            {
                if (value->has_pending_changes())
                {
                    return true;
                }
            }
            return false;
        }

        // Apply all pending changes
        void apply_all()
        {
            for (auto& [id, value] : m_values)
            {
                value->apply_changes();
            }
            m_has_changes = false;
            
            // Fire container-level callback
            if (m_on_applied)
            {
                m_on_applied();
            }
        }

        // Revert all pending changes
        void revert_all()
        {
            for (auto& [id, value] : m_values)
            {
                value->revert_changes();
            }
            m_has_changes = false;
        }

        // Check if container has any advanced values
        bool has_advanced_values() const
        {
            for (const auto& [id, value] : m_values)
            {
                if (value->is_advanced())
                {
                    return true;
                }
            }
            return false;
        }


        // Convenience methods for creating and adding values
        ImGuiToggle* add_toggle(const std::string& id, bool default_value = false, const std::string& label = "", const std::string& tooltip = "")
        {
            return add_value(id, ImGuiToggle::create(default_value, label.empty() ? id : label, tooltip));
        }

        ImGuiFloat* add_float(const std::string& id, float default_value = 0.0f, const std::string& label = "", const std::string& tooltip = "")
        {
            return add_value(id, ImGuiFloat::create(default_value, label.empty() ? id : label, tooltip));
        }

        ImGuiDouble* add_double(const std::string& id, double default_value = 0.0, const std::string& label = "", const std::string& tooltip = "")
        {
            return add_value(id, ImGuiDouble::create(default_value, label.empty() ? id : label, tooltip));
        }

        ImGuiSlider* add_slider(const std::string& id, float min, float max, float default_value, const std::string& label = "", const std::string& tooltip = "")
        {
            return add_value(id, ImGuiSlider::create(min, max, default_value, label.empty() ? id : label, tooltip));
        }

        ImGuiInt32* add_int(const std::string& id, int32_t default_value = 0, const std::string& label = "", const std::string& tooltip = "")
        {
            return add_value(id, ImGuiInt32::create(default_value, label.empty() ? id : label, tooltip));
        }

        ImGuiString* add_string(const std::string& id, const std::string& default_value = "", const std::string& label = "", const std::string& tooltip = "")
        {
            return add_value(id, ImGuiString::create(default_value, label.empty() ? id : label, 256, tooltip));
        }

        template<typename EnumType>
        ImGuiEnum<EnumType>* add_enum(const std::string& id, 
                                      const std::vector<typename ImGuiEnum<EnumType>::EnumOption>& options, 
                                      EnumType default_value, 
                                      const std::string& label = "",
                                      const std::string& tooltip = "")
        {
            return add_value(id, ImGuiEnum<EnumType>::create(options, default_value, label.empty() ? id : label, tooltip));
        }

        // Visibility control for tab systems
        bool is_visible() const { return m_visible; }
        void set_visible(bool visible) { m_visible = visible; }
        
        // Get unique ID for ImGui stacking
        int get_unique_id() const { return m_unique_id; }
        
        // Get/set title (useful for dynamic tab names)
        const std::string& get_title() const { return m_title; }
        void set_title(const std::string& title) { m_title = title; }
        
        // Check if container has any values
        bool empty() const { return m_values.empty(); }
        size_t size() const { return m_values.size(); }
        
        // Remove a value
        void remove_value(const std::string& id)
        {
            m_values.erase(id);
            m_value_order.erase(
                std::remove(m_value_order.begin(), m_value_order.end(), id),
                m_value_order.end()
            );
        }
        
        // Clear all values
        void clear()
        {
            m_values.clear();
            m_value_order.clear();
            m_has_changes = false;
        }
        
        // Container-level callbacks
        void set_on_value_changed_callback(ContainerChangedCallback callback) { m_on_value_changed = callback; }
        void set_on_applied_callback(ContainerAppliedCallback callback) { m_on_applied = callback; }
        
        // Global edit mode control
        void set_global_edit_mode(EditMode mode)
        {
            m_global_edit_mode = mode;
            m_apply_global_edit_mode = true;
            
            // Apply to all existing values
            for (auto& [id, value] : m_values)
            {
                value->set_edit_mode(mode);
            }
        }
        
        EditMode get_global_edit_mode() const { return m_global_edit_mode; }
        
        // Enable/disable the "Allow Editing" checkbox
        void show_edit_mode_control(bool show) { m_show_edit_mode_control = show; }
        bool is_showing_edit_mode_control() const { return m_show_edit_mode_control; }
        
        // Control whether global edit mode is applied to new values
        void set_apply_global_edit_mode(bool apply) { m_apply_global_edit_mode = apply; }

    private:
        static int generate_unique_id()
        {
            static int counter = 0;
            return ++counter;
        }

        std::string m_title;
        std::unordered_map<std::string, IImGuiValue::Ptr> m_values;
        std::vector<std::string> m_value_order; // Maintains insertion order
        bool m_show_advanced;
        bool m_has_changes;
        bool m_visible;
        int m_unique_id;
        EditMode m_global_edit_mode = EditMode::Editable;
        bool m_apply_global_edit_mode = false;
        bool m_show_edit_mode_control = false;
        ContainerChangedCallback m_on_value_changed;
        ContainerAppliedCallback m_on_applied;
    };

} // namespace RC::GUI