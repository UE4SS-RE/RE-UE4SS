#pragma once

#include <unordered_map>
#include <unordered_set>

#include <Unreal/FFrame.hpp>
#include <Unreal/UStruct.hpp>
#include <Unreal/UObject.hpp>

namespace RC::GUI::KismetDebuggerMod
{
    using namespace RC::Unreal;

    typedef void (*FNativeFuncPtr)(UObject* Context, FFrame& Stack, void* RESULT_DECL);

    auto expr_to_string(EExprToken expr) -> const char*;

    struct PausedContext
    {
        EExprToken expr{};
        UObject* context{};
        FFrame* stack{};
    };

    class BreakpointStore
    {
    public:
        BreakpointStore();
        ~BreakpointStore();

        auto load(std::filesystem::path& path) -> void;
        auto save() -> void;

        auto has_breakpoint(UFunction* fn, size_t index) -> bool;
        auto add_breakpoint(UFunction* fn, size_t index) -> void;
        auto add_breakpoint(const std::wstring& fn, size_t index) -> void;
        auto remove_breakpoint(UFunction* fn, size_t index) -> void;

    private:
        typedef std::unordered_set<size_t> FunctionBreakpoints;

        std::unordered_map<UFunction*, std::shared_ptr<FunctionBreakpoints> > m_breakpoints_by_function{};
        std::unordered_map<std::wstring, std::shared_ptr<FunctionBreakpoints> > m_breakpoints_by_name{};
    };

    class Debugger
    {
    public:
        Debugger();
        ~Debugger();

        auto enable() -> void;
        auto disable() -> void;

        auto render() -> void;
        auto render_nav_bar(float width) -> void;

        auto nav_to_function(UFunction* fn) -> void;
        auto nav_to_function(std::string full_name) -> void;

    private:
        bool m_paused{};

        // std::string because it's used primarily with ImGui
        std::unordered_map<std::string, UFunction*> m_function_name_map{};
        std::vector<std::string> m_nav_history{};
        int m_nav_history_index{-1};
        std::string m_nav_function{""};

        // layout
        float m_split_right{400.0};
        float m_split_left{400.0};

        uint8_t* m_last_code{nullptr}; // pointer to last stack instruction, used to know if it's advanced since last frame
        BreakpointStore& m_breakpoints;
    
    public:
        static inline std::filesystem::path m_save_path;
    };

    class ScriptRenderContext
    {
    public:
        ScriptRenderContext(std::optional<PausedContext>& paused_context, UFunction* fn, bool scroll_to_active, BreakpointStore& breakpoints);
        ~ScriptRenderContext();

        auto render() -> void;

    private:
        auto render_property();
        auto render_expr() -> EExprToken;

        template<typename T>
        auto read() -> T
        {
            T t;
            memcpy(&t, &m_script[m_index], sizeof(T));
            m_index += sizeof(T);
            return t;
        }

        auto read_object() -> UObject*
        {
            return (UObject*)this->read<uint64_t>();
        }
        auto read_name() -> FName
        {
            FName n = read<FName>();
            read<uint32_t>(); // not sure what this is for
            return n;
        }


    private:
        std::optional<PausedContext>& m_paused_context;
        UFunction* m_fn{};

        bool m_scroll_to_active{};
        BreakpointStore& m_breakpoints;

        int m_indent{0};

        uint8_t* m_script{};
        int m_script_size{};
        int m_index{};
        int m_cur{}; // or -1
        EExprToken m_current_expr{};
    };
}
