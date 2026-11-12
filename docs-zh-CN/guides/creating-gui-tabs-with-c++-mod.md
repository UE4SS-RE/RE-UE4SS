# 使用 C++ Mod 创建 GUI 标签页

> UE4SS 已经内置了 ImGui 库来渲染其控制台 GUI，构建自 [UE4SS-RE/imgui](https://github.com/UE4SS-RE/imgui) 仓库。关于如何使用 ImGui 特有的类与方法渲染实际的按钮、文本框及其他窗口对象，请参考[该仓库中的 ImGui 文档](https://github.com/UE4SS-RE/imgui/tree/master/docs)。

本指南将展示如何用 C++ mod 为 GUI 创建自定义标签页，并以代码示例中注释的形式说明：
```c++
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>

class MyAwesomeMod : public RC::CppUserModBase
{
private:
    int m_private_number{33};
    std::shared_ptr<GUI::GUITab> m_less_safe_tab{};

public:
    MyAwesomeMod() : CppUserModBase()
    {
        ModName = STR("MyAwesomeMod");
        ModVersion = STR("1.0");
        ModDescription = STR("This is my awesome mod");
        ModAuthors = STR("UE4SS Team");

        // 'register_tab' 函数会告诉 UE4SS 渲染一个标签页。
        // 以这种方式注册的标签页会在这个 C++ mod 被析构时自动清理。
        // 第一个参数是标签页的显示名称。
        // 第二个参数是 UE4SS 用来渲染标签页内容的回调。
        // 回调的参数是指向你的 mod 的指针。
        register_tab(STR("My Test Tab"), [](CppUserModBase* instance) {
            // 在这个回调中，你可以开始用 ImGui 渲染标签页的内容。
            ImGui::Text("This is the contents of the tab");

            // 你可以通过 'instance' 参数访问 mod 类的成员。
            auto mod = dynamic_cast<MyAwesomeMod*>(instance);
            if (!mod)
            {
                // 出了问题，导致 'instance' 没有被正确设置。
                // 中止函数剩余部分，以免访问无效指针。
                return;
            }

            // 公有与私有成员都可以访问。
            mod->render_some_stuff(mod->m_private_number);
        });

        // 'UE4SSProgram::add_gui_tab' 函数是告诉 UE4SS 渲染标签页的另一种方式。
        // 用这种方式注册标签页，你需要自己负责在 mod 析构时清理它。
        // 若在 mod 析构时未能清理标签页，会导致崩溃。
        // 推荐使用 'register_tab' 而不是该函数。
        m_less_safe_tab = std::make_shared<GUI::GUITab>(STR("My Less Safe Tab"), [](CppUserModBase* instance) {
            // 该回调与 'register_tab' 使用的回调完全相同，只是 'instance' 始终为 nullptr。
            ImGui::Text("This is the contents of the less safe tab");
        });
        UE4SSProgram::get_program().add_gui_tab(m_less_safe_tab);
    }

    ~MyAwesomeMod() override
    {
        // 因为你用 'UE4SSProgram::add_gui_tab' 创建了标签页，必须手动移除它。
        // 若未移除标签页，会导致崩溃。
        UE4SSProgram::get_program().remove_gui_tab(m_less_safe_tab);
    }

    auto on_ui_init() -> void override
    {
        // 如果你打算在 UE4SS 的上下文中使用 ImGui，启用 ImGui 是至关重要的。
        // 如果不这样做，一旦 ImGui 尝试渲染任何内容（例如你的标签页），就会崩溃。
        UE4SS_ENABLE_IMGUI()
    }

    auto render_some_stuff(int Number) -> void
    {
        auto calculated_value = Number + 1;
        ImGui::Text(std::format("calculated_value: {}", calculated_value).c_str());
    }
};

#define MY_AWESOME_MOD_API __declspec(dllexport)
extern "C"
{
    MY_AWESOME_MOD_API RC::CppUserModBase* start_mod()
    {
        return new MyAwesomeMod();
    }

    MY_AWESOME_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
```
