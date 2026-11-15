# 实时查看器

实时查看器（Live Viewer）是一个可以搜索、查看、编辑并监视所有对象属性的工具，因此在调试 mod 或分析数值在运行时如何变化时非常有用。但请注意，它无法显示未被反射的数据。

要显示它，你必须确保以下配置项均被设置为 1：

- `GuiConsoleEnabled`
- `GuiConsoleVisible`

**如果你在显示窗口时遇到问题，例如窗口打开后是一片空白/白色，可以修改 `GraphicsAPI` 设置，例如改为 `dx11`。**

有时字体太小不易看清（尤其是在较高分辨率下）。你可以修改 `GuiConsoleFontScaling` 设置来调整。

![live-viewer](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/221ccc8c-b49f-47a5-81b5-59dbbe85cbd1)

你可以右键点击搜索栏来显示选项。

![live-viewer search options](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/11612a38-0d00-4700-8a41-f6649c8174c0)

| 设置 | 说明 |
|------|------|
| Refresh search | 刷新搜索结果。 |
| Save filters | 将当前的搜索过滤器保存到 `工作目录/liveview/filters.meta.json`。下次启动游戏时会加载已保存的过滤器。 |
| Apply when not searching | 在未按名称搜索任何对象时也应用这些过滤器。 |
| Include inheritance | 包含搜索结果的任意子对象。 |
| Instances only | 仅包含对象实例。这些是作为 actor 的一部分存在于关卡中的对象（actor 本身、控件、组件等），其属性反映的是实时数值。例如 `<package name>_C` 或 `<package name>_C_<some instance id>`。 |
| Non-instances only | 仅包含对象包的默认状态，它们被加载到内存中但并不存在于关卡里。你无法修改这些属性的值。 |
| CDOs only | 仅包含类默认对象（CDO），即非实例对象从 UClass 继承的反射属性。例如 `<full path>_GEN_VARIABLE` 或 `<package path>.Default__<package name>`。 |
| Include CDOs | 在任意搜索条件中包含 CDO。 |
| Use Regex for search | 允许使用正则表达式进行更精确的搜索。 |
| Match Memory Address | 当搜索内容是一个合法的十六进制内存地址时，把位于该地址的对象、或在该地址拥有属性的对象纳入结果。 |
| Exclude class names | 允许排除特定的类名，例如 `CanvasPanelSlot` 或 `StaticMeshComponent`。使用逗号分隔，例如 `CanvasPanelSlot, StaticMeshComponent,Package , Function`。 |
| Include class names | 通过排除类名的下拉菜单访问。允许只包含特定的类名，例如 `CanvasPanelSlot` 或 `StaticMeshComponent`。使用逗号分隔，例如 `CanvasPanelSlot, StaticMeshComponent,Package , Function`。 |
| Has property | 仅查找具有指定名称属性的对象，例如 `Player Name`。只有在搜索栏中输入了内容时该设置才会生效。 |
| Has property type | 仅查找具有指定类型属性的对象，例如 `BoolProperty` 或 `MulticastInlineDelegateProperty`。 |
| Function parameter flags | 点击该复选框会打开一个新弹窗。允许你选择任意数量的标志来过滤函数，以及是否使用所选标志去检查返回值标志。 |

在底部的属性查看器面板中，有三个子控件：

- `<<` 在浏览历史中后退。
- `>>` 在浏览历史中前进。
- `Find functions` 打开一个窗口，用于搜索并调用与该对象及其所有子对象关联的任意函数。你可以用它来测试在游戏内调用函数，而无需编写任何代码。

![property view controls](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/551f9efd-5a68-4e7b-b11c-5fa72955768f)
![find functions](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/58ec56ac-d4b9-402b-9f64-9e41c8270bb5)

## 监视（Watches）

你可以右键点击大多数属性类型以及函数，来添加一个监视项。

![add watch](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/ea7d8a0f-4966-44bc-a08d-7a197e4e6195)

要查看你的监视项，请切换到 watches 标签页。

点击每个监视项左侧的加号按钮可展开数值框。

![expand watch values](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/992eb214-304c-428a-b173-920511faf4e0)

监视项左侧有两个复选框：

- 启用/禁用该监视项
- 将监视项写入文件。若启用，关闭游戏时监视项中的数值会保存为文本文件，存放在 `工作目录/watches/` 中。

监视项右侧有一个保存选项，保存后在你重新启动游戏时会自动重新添加该监视项。这些数据存储在 `工作目录/watches/watches.meta.json` 中。

![save watch](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/8b223d1d-a77f-4222-b287-89da12529c89)
![method of watch re-aquisition](https://github.com/UE4SS-RE/RE-UE4SS/assets/84156063/6cea33d9-73ae-416c-9943-e71553a6433d)
