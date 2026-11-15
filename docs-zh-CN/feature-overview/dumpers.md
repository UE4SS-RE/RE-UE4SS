# 转储工具

## C++ 头文件生成器

C++ 转储器是一个根据 UE4 类和蓝图生成 C++ 头文件的工具。

生成头文件的快捷键默认为 `CTRL` + `H`，可以在 `Mods/Keybinds/Scripts/main.lua` 中修改。

它会为每个蓝图（包括动画蓝图和控件蓝图）生成一个 `.hpp` 文件，随后把所有基类生成到 `<ProjectName>.hpp` 或 `<EngineModule>.hpp` 中。所有类位于文件顶部，其后是所有结构体。枚举会被单独生成到与其所属类同名的文件中，但文件名末尾追加 `_enums`。

### 配置项

- `DumpOffsetsAndSizes`（bool）
    - 是否输出属性的偏移与大小
    - 默认值：1

- `KeepMemoryLayout`（bool）
    - 类与结构体的内存布局是否应保持精确
    - 如果你想在真实的 C++ 项目中使用生成的头文件，必须将其设为 1
    - 设为 0 时，不会生成用于填充的成员变量
    - 默认值：0
    > 警告：目前设为 1 并没有实际意义，因为无论哪种方式内存数值都不准确！

- `LoadAllAssetsBeforeGeneratingCXXHeaders`（bool）
    - 是否在生成头文件前强制加载所有资源
    - 默认值：0
    > 警告：可能额外占用数 GB 内存，且不稳定；若你在转储后越过主菜单继续游玩，游戏会崩溃

## Unreal Header Tool (UHT) 转储器

为你的游戏生成与 Unreal Header Tool 兼容的 C++ 头文件，以便创建一个镜像 `.uproject`。关于如何使用这些头文件的指南见[此处](../guides/generating-uht-compatible-headers.md)。

生成头文件的快捷键默认为 `CTRL` + `Numpad 9`，可以在 `Mods/Keybinds/Scripts/main.lua` 中修改。

### 配置项

- `IgnoreAllCoreEngineModules`（bool）
    - 是否跳过生成属于引擎的包，对修改过引擎的游戏尤其有用
    - 默认值：0

- `IgnoreEngineAndCoreUObject`（bool）
    - 是否跳过生成 `Engine` 与 `CoreUObject` 包
    - 默认值：1

- `MakeAllFunctionsBlueprintCallable`（bool）
    - 是否强制所有 UFUNCTION 宏带有 `BlueprintCallable`
    - 默认值：1
    > 警告：这会导致生成的头文件中出现一些错误，你需要手动修复

- `MakeAllPropertyBlueprintsReadWrite`（bool）
    - 是否强制所有 UPROPERTY 宏带有 `BlueprintReadWrite`
    - 同时还会强制所有 UPROPERTY 宏带有 `meta=(AllowPrivateAccess=true)`
    - 默认值：1

- `MakeEnumClassesBlueprintType`（bool）
    - 当枚举的底层类型为隐式类型或 uint8 时，是否强制其 UENUM 宏带有 `BlueprintType`
    - 默认值：1
    > 警告：这还会把原本为隐式类型的底层类型强制指定为 uint8

- `MakeAllConfigsEngineConfig`（bool）
    - 是否对使用了 `DefaultConfig`、`GlobalUserConfig` 或 `ProjectUserConfig` 中任意一个的所有 UCLASS 宏强制设置 `Config = Engine`
    - 默认值：1

## 对象转储器

将所有已加载的对象转储到文件 `UE4SS_ObjectDump.txt` 中（你可以开启对所有资源的强制加载）。

转储对象的快捷键默认为 `CTRL` + `J`，可以在 `Mods/Keybinds/Scripts/main.lua` 中修改。

输出示例：

```
[000002A70F57E5C0] Function /Game/UI/Art/WidgetParts/Basic_ButtonScalable2.Basic_ButtonScalable2_C:BndEvt__Button_0_K2Node_ComponentBoundEvent_0_OnButtonClickedEvent__DelegateSignature [n: 5343AA] [c: 000002A727993A00] [or: 000002A708466980]
[000002A70F57E4E0] Function /Game/UI/Art/WidgetParts/Basic_ButtonScalable2.Basic_ButtonScalable2_C:PreConstruct [n: 4057B] [c: 000002A727993A00] [or: 000002A708466980]
[000002A70F876600] BoolProperty /Game/UI/Art/WidgetParts/Basic_ButtonScalable2.Basic_ButtonScalable2_C:PreConstruct:IsDesignTime [o: 0] [n: 4D63DB] [c: 00007FF683722CC0] [owr: 000002A70F57E4E0]
```

其中有多组方括号，每一组都有不同的含义，下表说明了各字母代表什么。
第一组方括号内是对象或属性所在的内存位置。

| 字母 | 含义 | UE 成员变量 |
|------|------|-------------|
| n | 对象/属性的名称 | NamePrivate |
| c | 对象/属性/枚举值所属的类 | ClassPrivate |
| or | 对象的 Outer | OuterPrivate |
| o | 属性值在对象中的偏移 | Offset_Internal |
| owr | FField 的拥有者，仅 4.25+ | Owner |
| kp | FMapProperty 的键属性 | KeyProp |
| vp | FMapProperty 的值属性 | ValueProp |
| mc | FClassProperty 所引用的类 | MetaClass |
| df | FDelegateProperty 所引用的函数 | FunctionSignature |
| pc | FObjectProperty/FFieldPathProperty 所引用的类 | PropertyClass |
| ic | FInterfaceProperty 所引用的类 | InterfaceClass |
| ss | FStructProperty 所引用的结构体 | Struct |
| em | FEnumProperty 所引用的枚举 | Enum |
| fm | FBoolProperty 所引用的字段掩码 | FieldMask |
| bm | FBoolProperty 所引用的字节掩码 | ByteMask |
| v | 该枚举键对应的值 | N/A |
| sps | 该 UClass 的 SuperStruct | SuperStruct |
| ai | FArrayProperty 存储的属性 | Inner |
| f | 指向 UFunction 的原生函数包装器；若非原生函数则为 ProcessInternal | Func |

### 配置项

- `LoadAllAssetsBeforeDumpingObjects`（bool）
    - 是否在转储对象前强制加载所有资源
    - 默认值：0
    > 警告：可能额外占用数 GB 内存，且不稳定；若你在转储后越过主菜单继续游玩，游戏会崩溃

## .usmap 转储器

为未版本化的属性生成 `.usmap` 映射文件。

转储映射的快捷键默认为 `Ctrl` + `Numpad 6`，可以在 `Mods/Keybinds/Scripts/main.lua` 中修改。

默认情况下，转储内容除了原生类型外还会包含蓝图生成的类、结构体和枚举。可以在 GUI 的 `Dumpers` 标签页中取消勾选 `Include Blueprint-generated types`，或在 Lua 中调用 `DumpUSMAP(false)` 来关闭该行为。

感谢 [OutTheShade](https://github.com/OutTheShade/UnrealMappingsDumper) 提供了最初的实现。

## .jmap 转储器

生成 `.jmap` 反射转储（JSON 格式，由 [trumank](https://github.com/trumank/jmap) 设计）。

转储内容包含类、函数、结构体和枚举的完整反射数据（包括属性偏移、大小与标志），类默认对象的属性值，以及近似的虚表布局。jmap 命令行工具可以将其转换为 `.usmap` 或 C++ 头文件，`ue_binja` 插件则可以把它应用到 Binary Ninja 数据库中。

转储的快捷键默认为 `Ctrl` + `Numpad 5`，可以在 `Mods/Keybinds/Scripts/main.lua` 中修改。也可以通过 GUI 的 `Dumpers` 标签页或 Lua 函数 `DumpJMAP()` 触发。

默认情况下，转储内容除了原生对象外还会包含蓝图生成的类、结构体和枚举（以及它们的 CDO）。可以在 GUI 的 `Dumpers` 标签页中取消勾选 `Include Blueprint-generated types`，或在 Lua 中调用 `DumpJMAP(false)` 来关闭该行为。

文件会写入 UE4SS 的工作目录，文件名为 `<GameName>-<EngineVersion>-<UE4SSCommitSHA>.jmap`。

## .umap 重建转储器

将所有已加载的 actor 转储到文件 `ue4ss_static_mesh_data.csv` 中，以便在编辑器内生成 `.umaps`。

要在编辑器内加载转储的 actor 以重建 `.umap`，需要满足两个前提：

- 所有被转储的 actor（静态网格体及其材质/贴图）都必须在编辑器中重建
- 从 Releases 页面下载 `zMapGenBP.zip`，并按照其中 Readme 文件的说明操作

转储的快捷键默认为 `Ctrl` + `Numpad 7`，可以在 `Mods/Keybinds/Scripts/main.lua` 中修改。
