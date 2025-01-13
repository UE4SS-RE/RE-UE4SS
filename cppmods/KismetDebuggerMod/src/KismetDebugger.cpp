#include <KismetDebugger.hpp>

#include <vector>
#include <unordered_map>
#include <iostream>
#include <thread>
#include <ranges>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/FString.hpp>
#include <Unreal/Core/Containers/Array.hpp>
#include <Unreal/FFrame.hpp>
#include <Unreal/Script.hpp>
#include <Unreal/ReflectedFunction.hpp>
#include <Unreal/Signatures.hpp>
#include <Unreal/Property/FObjectProperty.hpp>

#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>
#include <glaze/glaze.hpp>

#include "Profiler/Profiler.hpp"

#include <UE4SSProgram.hpp>

namespace RC::GUI::KismetDebuggerMod
{

    using namespace RC::Unreal;

    FNativeFuncPtr GNativesOriginal[EExprToken::EX_Max];
    volatile bool is_hooked = false; // cannot hook *immediately* as GNatives is populated at runtime

    volatile bool should_pause = false;
    volatile bool should_next = false;
    std::optional<PausedContext> context;
    std::mutex context_mutex;

    BreakpointStore g_breakpoints;

    void hook_expr_internal(UObject* Context, FFrame& Stack, void* RESULT_DECL, EExprToken N) {
        UFunction* fn = Stack.Node();
        StringType name = Stack.Node()->GetFullName();
        ProfilerTransientScopeNamed(scope, to_string(name).c_str(), true);
            
        size_t index = Stack.Code() - fn->GetScript().GetData() - 1;
        if (should_pause || g_breakpoints.has_breakpoint(fn, index))
        {
            should_pause = true;
            std::unique_lock<std::mutex> lock_a(context_mutex);
            PausedContext ctx{
                .expr = N,
                .context = Context,
                .stack = &Stack,
            };
            context = std::optional<PausedContext>(ctx);
            lock_a.unlock();
            while (should_pause && !should_next)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            std::unique_lock<std::mutex> lock_b(context_mutex);
            context = std::nullopt;
            lock_b.unlock();
            should_next = false;
        }

        GNativesOriginal[N](Context, Stack, RESULT_DECL);
    }
    
    template <unsigned N>
    void hook_expr(UObject* Context, FFrame& Stack, void* RESULT_DECL) {
        hook_expr_internal(Context, Stack, RESULT_DECL, static_cast<EExprToken>(N));
    }

    template <unsigned N> void hook_all() {
        GNativesOriginal[N - 1] = GNatives_Internal[N - 1];
        GNatives_Internal[N - 1] = &hook_expr<N - 1>;
        hook_all<N - 1>();
    }
    template <> void hook_all<0>() {}

    typedef std::unordered_map<std::string, std::unordered_set<size_t>> JsonBreakpoints;

    BreakpointStore::BreakpointStore()
    {
    }
    BreakpointStore::~BreakpointStore()
    {
    }
    auto BreakpointStore::load(std::filesystem::path& path) -> void
    {
        JsonBreakpoints breakpoints{};
        auto ec = glz::read_file_json(breakpoints, path.string(), std::string{});

        for (const auto& [fn, bps] : breakpoints)
        {
            auto wfn = ensure_str(fn);
            for (const auto& bp : bps)
            {
                add_breakpoint(wfn, bp);
            }
        }
    }
    auto BreakpointStore::save() -> void
    {
            JsonBreakpoints breakpoints{};
            for (const auto& [fn, bps] : m_breakpoints_by_name) {
                if (bps) breakpoints[to_string(fn)] = *bps;
            }
            auto ec = glz::write_file_json(breakpoints, Debugger::m_save_path.string(), std::string{});

    }
    
    auto BreakpointStore::has_breakpoint(UFunction* fn, size_t index) -> bool
    {
        auto [it_fn, inserted] = m_breakpoints_by_function.emplace(fn, nullptr);
        if (!inserted)
        {
            if (it_fn->second)
                return it_fn->second->contains(index);
        }
        else
        {
            // insert null so we know we've already inserted the fn ptr into the name map
            auto [it_name, inserted] = m_breakpoints_by_name.emplace(fn->GetFullName(), nullptr);
            if (!inserted)
            {
                if (it_name->second)
                {
                    it_fn->second = it_name->second;
                    return it_name->second->contains(index);
                }
            }
        }
        return false;
    }
    auto BreakpointStore::add_breakpoint(UFunction* fn, size_t index) -> void
    {
        std::shared_ptr<FunctionBreakpoints> bps;
        auto [it_fn, inserted_fn] = m_breakpoints_by_function.emplace(fn, nullptr);
        auto [it_name, inserted_name] = m_breakpoints_by_name.emplace(fn->GetFullName(), nullptr);
        if (!inserted_fn && it_fn->second) bps = it_fn->second;
        if (!inserted_name && it_name->second) bps = it_name->second;

        if (!bps)
            bps = it_fn->second = it_name->second = std::make_shared<FunctionBreakpoints>();

        bps->emplace(index);

        save();

        /*
        std::cout << "all breakpoints by fn ptr:" << std::endl;
        for (const auto& fns : m_breakpoints_by_function) {
            std::cout << (void*) fns.first << " = ";
            if (fns.second)
                for (const auto& bps : *fns.second) {
                    std::cout << bps << " ";
                }
            else
                std::cout << "null";
            std::cout << std::endl;
        }

        std::cout << "all breakpoints by fn name:" << std::endl;
        for (const auto& fns : m_breakpoints_by_name) {
            std::wcout << fns.first << " = ";
            if (fns.second)
                for (const auto& bps : *fns.second) {
                    std::cout << bps << " ";
                }
            else
                std::cout << "null";
            std::cout << std::endl;
        }

        std::cout << "all non-null breakpoints by fn name:" << std::endl;
        for (const auto& fns : m_breakpoints_by_name) {
            if (fns.second)
            {
                std::wcout << fns.first << " = ";
                for (const auto& bps : *fns.second) {
                    std::cout << bps << " ";
                }
                std::cout << std::endl;
            }
        }
        */
    }
    auto BreakpointStore::add_breakpoint(const StringType& fn, size_t index) -> void
    {
        std::shared_ptr<FunctionBreakpoints> bps;
        auto [it_name, inserted_name] = m_breakpoints_by_name.emplace(fn, nullptr);
        if (!inserted_name && it_name->second) bps = it_name->second;

        if (!bps)
            bps = it_name->second = std::make_shared<FunctionBreakpoints>();

        bps->emplace(index);

        save();
    }
    auto BreakpointStore::remove_breakpoint(UFunction* fn, size_t index) -> void
    {
        std::shared_ptr<FunctionBreakpoints> bps;
        auto [it_fn, inserted_fn] = m_breakpoints_by_function.emplace(fn, nullptr);
        auto [it_name, inserted_name] = m_breakpoints_by_name.emplace(fn->GetFullName(), nullptr);
        if (!inserted_fn && it_fn->second) bps = it_fn->second;
        if (!inserted_name && it_fn->second) bps = it_name->second;

        if (bps)
            bps->erase(index);

        save();
    }

    Debugger::Debugger() : m_breakpoints(g_breakpoints)
    {
        m_save_path = StringType{UE4SSProgram::get_program().get_working_directory()} + fmt::format(STR("\\Mods\\KismetDebugger\\config\\breakpoints.json"));
    }
    Debugger::~Debugger()
    {
        if (is_hooked)
        {
            for (int i = 0; i < EExprToken::EX_Max; i++)
            {
                GNatives_Internal[i] = GNativesOriginal[i];
            }
            is_hooked = false;
            should_pause = false;
            should_next = false;

            // Give main thread some time to exit hooked function. This can
            // possibly be called to unload the DLL, in which case bad things
            // will happen if the main thread is still inside the hook when it
            // disappears.
            // TODO: Need to find a way to guarantee the main thread has exited
            // the hook before continuing.
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    }

    auto ImGui_Splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size, const char* id_str = "##Splitter") -> bool
    {
        using namespace ImGui;
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiID id = window->GetID(id_str);
        ImRect bb;
        bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
        bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
        return SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
    }

    auto Debugger::enable() -> void
    {
        // do a bunch of setup on enable rather than mod init because a lot of things aren't ready at mod init

        // hack to delay breakpoint loading because working directory changes at some point during load
        try
        {
            m_breakpoints.load(m_save_path);
        }
        catch (std::exception& e)
        {
            Output::send<LogLevel::Warning>(STR("[KismetDebugger]: Failed to load breakpoints: {}\n"), ensure_str(e.what()));
        }

        if (GNatives_Internal != nullptr)
        {
            // finally actually enable the debugger
            hook_all<EExprToken::EX_Max>();
            is_hooked = true;
            return;
        }
        Output::send<LogLevel::Error>(STR("[KismetDebugger]: GNatives not found.\n"));
    }
    auto Debugger::disable() -> void
    {
        for (int i = 0; i < EExprToken::EX_Max; i++)
        {
            GNatives_Internal[i] = GNativesOriginal[i];
        }
        is_hooked = false;
        should_pause = false;
        should_next = false;
    }

    auto get_object_address(FProperty* property, EExprToken expr, auto in_context) -> void*
    {
        void* container{};
        if constexpr (std::is_same_v<std::remove_cvref_t<decltype(in_context)>, std::optional<PausedContext>>)
        {
            if (!in_context.has_value()) { return nullptr; };
            const auto& context = in_context.value();
            if (expr == EX_InstanceVariable)
            {
                container = context.context;
            }
            else if (expr == EX_LocalVariable)
            {
                container = context.stack->Locals();
            }
            else
            {
                return nullptr;
            }
        }
        else
        {
            container = in_context;
        }
        return *property->ContainerPtrToValuePtr<void*>(container);
    }

    static auto try_rendering_property_context_menu(UFunction* fn, int index, FProperty* property, EExprToken expr, auto context) -> void
    {
        if (property->IsA<FObjectProperty>())
        {
            if (auto data_ptr = get_object_address(property, expr, context); data_ptr)
            {
                auto popup_context_name = to_string(fmt::format(STR("{}-{}-{}"), static_cast<void*>(fn), index, static_cast<void*>(property)));
                if (ImGui::BeginPopupContextItem(popup_context_name.c_str()))
                {
                    if (ImGui::MenuItem("Copy address"))
                    {
                        ImGui::SetClipboardText(fmt::format("{}", data_ptr).c_str());
                    }
                    ImGui::EndPopup();
                }
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("0x%p", data_ptr);
                    ImGui::EndTooltip();
                }
            }
        }
    }

    auto Debugger::render() -> void
    {
        std::scoped_lock lock(context_mutex);

        bool position_updated = context && m_last_code != context->stack->Code();
        if (position_updated)
            nav_to_function(context->stack->Node());

        UFunction* current_fn = nullptr;
        if (m_nav_history_index >= 0)
        {
            auto name = m_nav_history[m_nav_history_index];
            if (auto it = m_function_name_map.find(name); it != m_function_name_map.end())
                current_fn = it->second;
        }


        float m_split_right = (ImGui::GetContentRegionMax().x - m_split_left);

        ImGui_Splitter(true, 4.0f, &m_split_left, &m_split_right, 200.0f, 200.0f, -24.0f, "KismetDebugger_split");

        ImGui::BeginChild("KismetDebugger_Controls", {m_split_left - 2, -24.0f}, true);

        if (ImGui::Button(is_hooked ? "disable" : "enable"))
        {
            if (is_hooked)
            {
                disable();
            }
            else
            {
                enable();
            }
        }
        if (is_hooked)
        {
            if (auto ctx = context)
            {
                if (ImGui::Button("continue"))
                {
                    should_pause = false;
                }
                ImGui::SameLine();
                if (ImGui::Button("next"))
                {
                    should_next = true;
                }

                UFunction* node = ctx->stack->Node();
                size_t index = ctx->stack->Code() - node->GetScript().GetData() - 1;
                ImGui::Text("paused @ %s", expr_to_string(ctx->expr));
                ImGui::Text("index @ %zi", index);
                ImGui::Text("object context = %s", to_string(ctx->context->GetFullName()).c_str());

                if (ImGui::CollapsingHeader("Call stack", ImGuiTreeNodeFlags_DefaultOpen))
                {
                    int lines = 0;
                    FFrame* current = ctx->stack;
                    while (current != nullptr)
                    {
                        current = current->PreviousFrame();
                        lines++;
                    }
                    ImGui::BeginChild("KismetDebugger_CallStack", {0, ImGui::GetStyle().ScrollbarSize + lines * ImGui::GetTextLineHeightWithSpacing()}, false, ImGuiWindowFlags_HorizontalScrollbar);
                    current = ctx->stack;
                    for (int i = 0; current != nullptr; ++i)
                    {
                        ImGui::Text("%i: %s", i, to_string(current->Node()->GetFullName()).c_str());
                        current = current->PreviousFrame();
                    }
                    ImGui::EndChild();
                }
            }
            else
            {
                if (ImGui::Button("pause"))
                {
                    should_pause = true;
                }
            }
        }

        if (current_fn)
        {
            if (ImGui::CollapsingHeader("Locals", ImGuiTreeNodeFlags_DefaultOpen))
            {
                ImGui::BeginChild("KismetDebugger_Locals", {0, 0}, false, ImGuiWindowFlags_HorizontalScrollbar);
                if (context && context->stack->Node() == current_fn)
                {
                    UFunction* node = context->stack->Node();
                    int property_count{};
                    for (FProperty* property : current_fn->ForEachProperty())
                    {
                        FString text{};
                        auto container_ptr = property->ContainerPtrToValuePtr<void*>(context->stack->Locals());
                        property->ExportTextItem(text, container_ptr, container_ptr, static_cast<UObject*>(node), NULL);

                        ImGui::Text("%s = %S", to_string(property->GetName()).c_str(), text.GetCharArray());
                        try_rendering_property_context_menu(node, property_count, property, EX_Nothing, context->stack->Locals());
                        ++property_count;
                    }
                }
                else
                {
                    for (FProperty* property : current_fn->ForEachProperty())
                    {
                        ImGui::Text("%s", to_string(property->GetName()).c_str());
                    }
                }
                ImGui::EndChild();
            }
        }
        ImGui::EndChild();

        // left pane
        ImGui::SameLine(0, 8);

        ImGui::BeginGroup();

        render_nav_bar(m_split_right - 20);

        ImGui::BeginChild("KismetDebugger_Disassembly", {m_split_right - 20 , -24.0f}, true);


        if (current_fn)
        {
            ScriptRenderContext render_ctx{context, current_fn, position_updated, m_breakpoints};
            render_ctx.render();
        }

        ImGui::EndChild();

        ImGui::EndGroup(); // left pane


        if (is_hooked)
        {
            if (auto ctx = context)
            {
                m_last_code = ctx->stack->Code();
            }
        }

        if (!is_hooked || !context)
            m_last_code = nullptr;
    }

    auto Debugger::render_nav_bar(float width) -> void
    {
        float start = ImGui::GetCursorPosX();

        bool disable_back = m_nav_history_index < 0;
        if (disable_back)
            ImGui::BeginDisabled();
        if (ImGui::Button("back"))
        {
            m_nav_history_index--;
            m_nav_function = m_nav_history_index < 0 ? "" : m_nav_history[m_nav_history_index];
        }
        if (disable_back)
            ImGui::EndDisabled();

        ImGui::SameLine();
        bool disable_forward = m_nav_history_index + 1 >= m_nav_history.size();
        if (disable_forward)
            ImGui::BeginDisabled();
        if (ImGui::Button("forward"))
        {
            m_nav_history_index++;
            m_nav_function = m_nav_history[m_nav_history_index];
        }
        if (disable_forward)
            ImGui::EndDisabled();


        // https://github.com/ocornut/imgui/issues/718#issuecomment-1249822993
        ImGui::SameLine();
        ImGui::SetNextItemWidth(width - (ImGui::GetCursorPosX() - start));
        const bool is_input_text_enter_pressed = ImGui::InputText("##input", &m_nav_function, ImGuiInputTextFlags_EnterReturnsTrue);
        const bool is_input_text_active = ImGui::IsItemActive();
        const bool is_input_text_activated = ImGui::IsItemActivated();

        if (is_input_text_activated)
            ImGui::OpenPopup("##popup");

        {
            ImGui::SetNextWindowPos(ImVec2(ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y));
            //ImGui::SetNextWindowSize({ ImGui::GetItemRectSize().x, 0 });
            if (ImGui::BeginPopup("##popup", ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_ChildWindow))
            {
                int n = 0;

                UObjectGlobals::ForEachUObject([&](UObject* object, ...) {
                    if (object->IsA<UFunction>())
                    {
                        std::string full_name = to_string(object->GetFullName());

                        auto it = std::search(
                            full_name.begin(), full_name.end(),
                            m_nav_function.begin(), m_nav_function.end(),
                            [](auto ch1, auto ch2) { return std::toupper(ch1) == std::toupper(ch2); }
                        );

                        if (it != full_name.end())
                        {
                            const char* name_c_str = full_name.c_str();

                            if (ImGui::Selectable(name_c_str))
                            {
                                ImGui::ClearActiveID();
                                m_function_name_map[full_name] = static_cast<UFunction*>(object);
                                m_nav_function = full_name;
                            }

                            if (n++ > 10)
                            {
                                ImGui::Text("...");
                                return LoopAction::Break;
                            }
                        }
                    }
                    return LoopAction::Continue;
                });

                if (!n)
                    ImGui::Text("[nothing]");

                if (is_input_text_enter_pressed || (!is_input_text_active && !ImGui::IsWindowFocused()))
                {
                    ImGui::CloseCurrentPopup();
                    // TODO check valid

                    nav_to_function(m_nav_function);
                }

                ImGui::EndPopup();
            }
        }
    }

    auto Debugger::nav_to_function(UFunction* fn) -> void
    {
        std::string full_name = to_string(fn->GetFullName());

        m_function_name_map[full_name] = fn;

        nav_to_function(full_name);
    }

    // function name must already be in name map
    auto Debugger::nav_to_function(std::string full_name) -> void
    {
        if (m_nav_history_index >= 0 && m_nav_history[m_nav_history_index] == full_name)
            return; // already at fn, do not add another entry to the stack

        m_nav_function = full_name;

        m_nav_history_index++;
        if (m_nav_history_index >= m_nav_history.size())
            m_nav_history.push_back(m_nav_function);
        else
            m_nav_history[m_nav_history_index] = m_nav_function;
    }


    auto expr_to_string(EExprToken expr) -> const char*
    {
        switch (expr)
        {
            case EX_LocalVariable:
                return "EX_LocalVariable";
            case EX_InstanceVariable:
                return "EX_InstanceVariable";
            case EX_DefaultVariable:
                return "EX_DefaultVariable";
            case EX_Return:
                return "EX_Return";
            case EX_Jump:
                return "EX_Jump";
            case EX_JumpIfNot:
                return "EX_JumpIfNot";
            case EX_Assert:
                return "EX_Assert";
            case EX_Nothing:
                return "EX_Nothing";
            case EX_NothingInt32:
                return "EX_NothingInt32";
            case EX_Let:
                return "EX_Let";
            case EX_BitFieldConst:
                return "EX_BitFieldConst";
            case EX_ClassContext:
                return "EX_ClassContext";
            case EX_MetaCast:
                return "EX_MetaCast";
            case EX_LetBool:
                return "EX_LetBool";
            case EX_EndParmValue:
                return "EX_EndParmValue";
            case EX_EndFunctionParms:
                return "EX_EndFunctionParms";
            case EX_Self:
                return "EX_Self";
            case EX_Skip:
                return "EX_Skip";
            case EX_Context:
                return "EX_Context";
            case EX_Context_FailSilent:
                return "EX_Context_FailSilent";
            case EX_VirtualFunction:
                return "EX_VirtualFunction";
            case EX_FinalFunction:
                return "EX_FinalFunction";
            case EX_IntConst:
                return "EX_IntConst";
            case EX_FloatConst:
                return "EX_FloatConst";
            case EX_StringConst:
                return "EX_StringConst";
            case EX_ObjectConst:
                return "EX_ObjectConst";
            case EX_NameConst:
                return "EX_NameConst";
            case EX_RotationConst:
                return "EX_RotationConst";
            case EX_VectorConst:
                return "EX_VectorConst";
            case EX_Vector3fConst:
                return "EX_Vector3fConst";
            case EX_ByteConst:
                return "EX_ByteConst";
            case EX_IntZero:
                return "EX_IntZero";
            case EX_IntOne:
                return "EX_IntOne";
            case EX_True:
                return "EX_True";
            case EX_False:
                return "EX_False";
            case EX_TextConst:
                return "EX_TextConst";
            case EX_NoObject:
                return "EX_NoObject";
            case EX_TransformConst:
                return "EX_TransformConst";
            case EX_IntConstByte:
                return "EX_IntConstByte";
            case EX_NoInterface:
                return "EX_NoInterface";
            case EX_DynamicCast:
                return "EX_DynamicCast";
            case EX_StructConst:
                return "EX_StructConst";
            case EX_EndStructConst:
                return "EX_EndStructConst";
            case EX_SetArray:
                return "EX_SetArray";
            case EX_EndArray:
                return "EX_EndArray";
            case EX_PropertyConst:
                return "EX_PropertyConst";
            case EX_UnicodeStringConst:
                return "EX_UnicodeStringConst";
            case EX_Int64Const:
                return "EX_Int64Const";
            case EX_UInt64Const:
                return "EX_UInt64Const";
            case EX_DoubleConst:
                return "EX_DoubleConst";
            case EX_Cast: // EX_PrimitiveCast in 4.27
                return "EX_Cast";
            case EX_SetSet:
                return "EX_SetSet";
            case EX_EndSet:
                return "EX_EndSet";
            case EX_SetMap:
                return "EX_SetMap";
            case EX_EndMap:
                return "EX_EndMap";
            case EX_SetConst:
                return "EX_SetConst";
            case EX_EndSetConst:
                return "EX_EndSetConst";
            case EX_MapConst:
                return "EX_MapConst";
            case EX_EndMapConst:
                return "EX_EndMapConst";
            case EX_StructMemberContext:
                return "EX_StructMemberContext";
            case EX_LetMulticastDelegate:
                return "EX_LetMulticastDelegate";
            case EX_LetDelegate:
                return "EX_LetDelegate";
            case EX_LocalVirtualFunction:
                return "EX_LocalVirtualFunction";
            case EX_LocalFinalFunction:
                return "EX_LocalFinalFunction";
            case EX_LocalOutVariable:
                return "EX_LocalOutVariable";
            case EX_DeprecatedOp4A:
                return "EX_DeprecatedOp4A";
            case EX_InstanceDelegate:
                return "EX_InstanceDelegate";
            case EX_PushExecutionFlow:
                return "EX_PushExecutionFlow";
            case EX_PopExecutionFlow:
                return "EX_PopExecutionFlow";
            case EX_ComputedJump:
                return "EX_ComputedJump";
            case EX_PopExecutionFlowIfNot:
                return "EX_PopExecutionFlowIfNot";
            case EX_Breakpoint:
                return "EX_Breakpoint";
            case EX_InterfaceContext:
                return "EX_InterfaceContext";
            case EX_ObjToInterfaceCast:
                return "EX_ObjToInterfaceCast";
            case EX_EndOfScript:
                return "EX_EndOfScript";
            case EX_CrossInterfaceCast:
                return "EX_CrossInterfaceCast";
            case EX_InterfaceToObjCast:
                return "EX_InterfaceToObjCast";
            case EX_WireTracepoint:
                return "EX_WireTracepoint";
            case EX_SkipOffsetConst:
                return "EX_SkipOffsetConst";
            case EX_AddMulticastDelegate:
                return "EX_AddMulticastDelegate";
            case EX_ClearMulticastDelegate:
                return "EX_ClearMulticastDelegate";
            case EX_Tracepoint:
                return "EX_Tracepoint";
            case EX_LetObj:
                return "EX_LetObj";
            case EX_LetWeakObjPtr:
                return "EX_LetWeakObjPtr";
            case EX_BindDelegate:
                return "EX_BindDelegate";
            case EX_RemoveMulticastDelegate:
                return "EX_RemoveMulticastDelegate";
            case EX_CallMulticastDelegate:
                return "EX_CallMulticastDelegate";
            case EX_LetValueOnPersistentFrame:
                return "EX_LetValueOnPersistentFrame";
            case EX_ArrayConst:
                return "EX_ArrayConst";
            case EX_EndArrayConst:
                return "EX_EndArrayConst";
            case EX_SoftObjectConst:
                return "EX_SoftObjectConst";
            case EX_CallMath:
                return "EX_CallMath";
            case EX_SwitchValue:
                return "EX_SwitchValue";
            case EX_InstrumentationEvent:
                return "EX_InstrumentationEvent";
            case EX_ArrayGetByRef:
                return "EX_ArrayGetByRef";
            case EX_ClassSparseDataVariable:
                return "EX_ClassSparseDataVariable";
            case EX_FieldPathConst:
                return "EX_FieldPathConst";
            case EX_AutoRtfmTransact:
                return "EX_AutoRtfmTransact";
            case EX_AutoRtfmStopTransact:
                return "EX_AutoRtfmStopTransact";
            case EX_AutoRtfmAbortIfNot:
                return "EX_AutoRtfmAbortIfNot";
            case EX_Max:
                return "EX_Max";
        }
        return "unknown";
    };

    ScriptRenderContext::ScriptRenderContext(std::optional<PausedContext>& paused_context, UFunction* fn, bool scroll_to_active, BreakpointStore& breakpoints)
        : m_paused_context(paused_context),
        m_fn(fn),
        m_scroll_to_active(scroll_to_active),
        m_breakpoints(breakpoints),

        m_script(fn->GetScript().GetData()),
        m_script_size(fn->GetScript().Num()),
        m_cur((paused_context && paused_context->stack->Node() == fn) ? paused_context->stack->Code() - paused_context->stack->Node()->GetScript().GetData() - 1 : -1)
    {
    }
    ScriptRenderContext::~ScriptRenderContext()
    {
    }

    auto ScriptRenderContext::render() -> void
    {
        while (m_index < m_script_size) render_expr();
    }

    auto ScriptRenderContext::render_property()
    {
        FProperty* property = (FProperty*)read_object();
        if (property)
        {
            ImGui::Text("%s", to_string(property->GetName()).c_str());
            if (m_paused_context.has_value())
            {
                try_rendering_property_context_menu(m_fn, m_index, property, m_current_expr, m_paused_context);
            }
            /*
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();

                // TODO: this depends on context and isn't always a local
                FString text{};
                auto container_ptr = property->ContainerPtrToValuePtr<void*>(stack->Locals());
                property->ExportTextItem(text, container_ptr, container_ptr, static_cast<UObject*>(stack->Node()), NULL);

                ImGui::Text("= %S", text.GetCharArray());

                ImGui::EndTooltip();
            }
            */
        }
        else
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,0,0,255));
            ImGui::Text("NULL property");
            ImGui::PopStyleColor();
        }
    }
    auto ScriptRenderContext::render_expr() -> EExprToken
    {
        bool active = m_index == m_cur;
        size_t expr_index = m_index;

        m_current_expr = static_cast<EExprToken>(read<uint8>());

        //std::cout << "rendering (" << std::hex << unsigned(m_current_expr) << std::dec << ") @ " << (index - 1) << " " << expr_to_string(m_current_expr) << std::endl;


        ImGui::SetCursorPosX(ImGui::GetCursorStartPos().x);
        auto label = std::format("{}", expr_index);
        ImGui::PushStyleVar(ImGuiStyleVar_SelectableTextAlign, ImVec2(1.0f, 0.5f));
        bool is_breakpoint = m_breakpoints.has_breakpoint(m_fn, expr_index);
        if (ImGui::Selectable(label.c_str(), is_breakpoint, 0, {30, 0}))
        {
            if (is_breakpoint)
                m_breakpoints.remove_breakpoint(m_fn, expr_index);
            else
                m_breakpoints.add_breakpoint(m_fn, expr_index);
        }
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::SetCursorPosX(50 + m_indent * 20.0f);

        if (active)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255,255,0,255));
            if (m_scroll_to_active)
                ImGui::SetScrollHereY();
        }
        ImGui::Text("%s", expr_to_string(m_current_expr));
        if (active)
            ImGui::PopStyleColor();

        m_indent++;
        ImGui::SetCursorPosX(50 + m_indent * 20.0f);

        switch(m_current_expr)
        {
            case EX_Cast:
            {
                read<uint8>();
                render_expr();
                break;
            }
            case EX_ObjToInterfaceCast:
            case EX_CrossInterfaceCast:
            case EX_InterfaceToObjCast:
            {
                (UClass*)read_object();
                render_expr();
                break;
            }
            case EX_Let:
            {
                render_property();
            }
            case EX_LetObj:
            case EX_LetWeakObjPtr:
            case EX_LetBool:
            case EX_LetDelegate:
            case EX_LetMulticastDelegate:
            {
                render_expr();
                render_expr();
                break;
            }
            case EX_LetValueOnPersistentFrame:
            {
                render_property();
                render_expr();
                break;
            }
            case EX_StructMemberContext:
            {
                (FProperty*)read_object();        // struct member expr.
                render_expr();
                break;
            }
            case EX_Jump:
            {
                read<uint32>();
                break;
            }
            case EX_ComputedJump:
            {
                render_expr();
                break;
            }
            case EX_LocalVariable:
            case EX_InstanceVariable:
            case EX_DefaultVariable:
            case EX_LocalOutVariable:
            case EX_ClassSparseDataVariable:
            case EX_PropertyConst:
            {
                this->render_property();
                break;
            }
            case EX_InterfaceContext:
            {
                render_expr();
                break;
            }
            case EX_PushExecutionFlow:
            {
                read<uint32>();
                break;
            }
            case EX_NothingInt32:
            {
                read<int32>();
                break;
            }
            case EX_Nothing:
            case EX_EndOfScript:
            case EX_EndFunctionParms:
            case EX_EndStructConst:
            case EX_EndArray:
            case EX_EndArrayConst:
            case EX_EndSet:
            case EX_EndMap:
            case EX_EndSetConst:
            case EX_EndMapConst:
            case EX_IntZero:
            case EX_IntOne:
            case EX_True:
            case EX_False:
            case EX_NoObject:
            case EX_NoInterface:
            case EX_Self:
            case EX_EndParmValue:
            case EX_PopExecutionFlow:
            case EX_DeprecatedOp4A:
            {
                break;
            }
            case EX_WireTracepoint:
            case EX_Tracepoint:
            {
                break;
            }
            case EX_Breakpoint:
            {
                break;
            }
            /*
            case EX_InstrumentationEvent: TODO ??????
            {
                if (Script[iCode] == EScriptInstrumentation::InlineEvent)
                {
                    iCode += sizeof(FScriptName);
                }
                iCode += sizeof(uint8);
                break;
            }
            */
            case EX_Return:
            {
                render_expr();
                break;
            }
            case EX_CallMath:
            case EX_LocalFinalFunction:
            case EX_FinalFunction:
            {
                (UFunction*)read_object();
                while(render_expr() != EX_EndFunctionParms);
                break;
            }
            case EX_LocalVirtualFunction:
            case EX_VirtualFunction:
            {
                //std::cout << "reading " << sizeof(FName) << " bytes @ " << index << std::endl;

                /*
                for (int i = 0; i < script_size; ++i)
                    std::cout << script[i] << " ";
                std::cout << std::endl;
                */

                /*
                for (int i = 0; i < script_size; ++i)
                    std::cout << std::hex << std::setfill('0') << std::setw(2) << unsigned(script[i]) << " ";
                std::cout << std::endl;
                */

                FName n = read_name();
                while(render_expr() != EX_EndFunctionParms);    // Parms.
                break;
            }
            case EX_CallMulticastDelegate:
            {
                (UFunction*)read_object();
                while(render_expr() != EX_EndFunctionParms); // Parms.
                break;
            }
            case EX_BitFieldConst:
            {
                (FProperty*)read_object();
                read<uint8>();
                break;
            }
            case EX_ClassContext:
            case EX_Context:
            case EX_Context_FailSilent:
            {
                render_expr(); // Object expression.
                read<uint32>();
                (FField*)read_object();            // Property corresponding to the r-value data, in case the l-value needs to be mem-zero'd
                render_expr(); // Context expression.
                break;
            }
            case EX_AddMulticastDelegate:
            case EX_RemoveMulticastDelegate:
            {
                render_expr();    // Delegate property to assign to
                render_expr(); // Delegate to add to the MC delegate for broadcast
                break;
            }
            case EX_ClearMulticastDelegate:
            {
                render_expr();    // Delegate property to clear
                break;
            }
            case EX_IntConst:
            {
                ImGui::Text("%d", read<int32>());
                break;
            }
            case EX_Int64Const:
            {
                ImGui::Text("%lld", read<int64_t>());
                break;
            }
            case EX_UInt64Const:
            {
                ImGui::Text("%llu", read<uint64_t>());
                break;
            }
            case EX_DoubleConst:
            {
                ImGui::Text("%f", read<double>());
                break;
            }
            case EX_SkipOffsetConst:
            {
                ImGui::Text("%u", read<uint32>()); // TODO jump
                break;
            }
            case EX_FloatConst:
            {
                ImGui::Text("%f", read<float>());
                break;
            }
            case EX_StringConst:
            {
                while (read<uint8>());
                break;
            }
            case EX_UnicodeStringConst:
            {
                while (read<uint16>());
                break;
            }
            case EX_TextConst:
            {
                switch (read<uint8>())
                {
                    case 0: // Empty
                        break;
                    case 1: // LocalizedText
                        render_expr();
                        render_expr();
                        render_expr();
                        break;
                    case 2: // InvariantText
                        render_expr();
                        break;
                    case 3: // LiteralString
                        render_expr();
                        break;
                    case 4: // StringTableEntry
                        read_object();
                        render_expr();
                        render_expr();
                        break;
                }
                break;
            }
            case EX_ObjectConst:
            {
                read_object();
                break;
            }
            case EX_SoftObjectConst:
            {
                render_expr();
                break;
            }
            case EX_FieldPathConst:
            {
                render_expr();
                break;
            }
            case EX_NameConst:
            {
                FName n = read_name();
                break;
            }
            case EX_RotationConst:
            {
                read<int32>();
                read<int32>();
                read<int32>();
                break;
            }
            case EX_Vector3fConst:
            case EX_VectorConst:
            {
                read<float>();
                read<float>();
                read<float>();
                break;
            }
            case EX_TransformConst:
            {
                // Rotation
                read<float>(); read<float>(); read<float>(); read<float>();
                // Translation
                read<float>(); read<float>(); read<float>();
                // Scale
                read<float>(); read<float>(); read<float>();
                break;
            }
            case EX_StructConst:
            {
                (UScriptStruct*)read_object();    // Struct.
                read<int32>();
                while(render_expr() != EX_EndStructConst);
                break;
            }
            case EX_SetArray:
            {
                // If not loading, or its a newer version
                //if((!GetLinker()) || !Ar.IsLoading() || (Ar.UE4Ver() >= VER_UE4_CHANGE_SETARRAY_BYTECODE))
                //{
                    // Array property to assign to
                    EExprToken TargetToken = render_expr();
                //}
                //else
                //{
                    // Array Inner Prop
                    //(FProperty*)read_object();
                //}

                while(render_expr() != EX_EndArray);
                break;
            }
            case EX_SetSet:
                render_expr(); // set property
                read<int32>();
                while (render_expr() != EX_EndSet);
                break;
            case EX_SetMap:
                render_expr(); // map property
                read<int32>();
                while (render_expr() != EX_EndMap);
                break;
            case EX_ArrayConst:
            {
                (FProperty*)read_object();    // Inner property
                read<int32>();
                while (render_expr() != EX_EndArrayConst);
                break;
            }
            case EX_SetConst:
            {
                (FProperty*)read_object();    // Inner property
                read<int32>();
                while (render_expr() != EX_EndSetConst);
                break;
            }
            case EX_MapConst:
            {
                (FProperty*)read_object();    // Key property
                (FProperty*)read_object();    // Val property
                read<int32>();
                while (render_expr() != EX_EndMapConst);
                break;
            }
            case EX_ByteConst:
            case EX_IntConstByte:
            {
                read<int8>();
                break;
            }
            case EX_MetaCast:
            {
                (UClass*)read_object();
                render_expr();
                break;
            }
            case EX_DynamicCast:
            {
                (UClass*)read_object();
                render_expr();
                break;
            }
            case EX_JumpIfNot:
            {
                read<uint32>();
                render_expr(); // Boolean expr.
                break;
            }
            case EX_PopExecutionFlowIfNot:
            {
                render_expr(); // Boolean expr.
                break;
            }
            case EX_Assert:
            {
                read<uint16>();
                read<uint8>();
                render_expr(); // Assert expr.
                break;
            }
            case EX_Skip:
            {
                read<uint32>();
                render_expr(); // Expression to possibly skip.
                break;
            }
            case EX_InstanceDelegate:
            {
                FName n = read_name();
                break;
            }
            case EX_BindDelegate:
            {
                FName n = read_name();
                render_expr();    // Delegate property to assign to
                render_expr();
                break;
            }
            case EX_SwitchValue:
            {
                auto cases = read<uint16>(); // number of cases, without default one
                auto end = read<uint32>(); // Code offset, go to it, when done.
                render_expr();   //index term

                for (uint16 i = 0; i < cases; ++i)
                {
                    render_expr();   // case index value term
                    auto next_case = read<uint32>(); // offset to the next case
                    render_expr();   // case term
                }

                render_expr();   //default term
                break;
            }
            case EX_ArrayGetByRef:
            {
                render_expr();
                render_expr();
                break;
            }
            case EX_AutoRtfmTransact:
            {
                read<int32>();
                read<uint32>();
                while (render_expr() != EX_AutoRtfmStopTransact);
                break;
            }
            case EX_AutoRtfmStopTransact:
            {
                read<int32>();
                read<uint8>();
                break;
            }
            case EX_AutoRtfmAbortIfNot:
            {
                render_expr();
                break;
            }
            default:
            {
                // This should never occur.
                //UE_LOG(LogScriptSerialization, Warning, TEXT("Error: Unknown bytecode 0x%02X; ignoring it"), (uint8)Expr );
                std::cout << "unknown expr (" << unsigned(m_current_expr) << ") " << expr_to_string(m_current_expr) << std::endl;
                ImGui::Text("unknown expr (%i) %s", unsigned(m_current_expr), expr_to_string(m_current_expr));
                break;
            }
        }
        m_indent--;
        return m_current_expr;
    }

}
