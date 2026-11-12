# 修复缺失的 AOB

如果 UE4SS 因为缺少 AOB 而无法正常启动，你可以用 Lua 提供自己的 AOB 与回调。

本指南需要你了解什么是 `根目录`（root directory）与 `工作目录`（working directory）。
`根目录`始终是包含 `ue4ss.dll` 的目录。
`工作目录`要么是包含 `ue4ss.dll` 的目录，要么是特定于游戏的目录，例如 `<根目录>/SatisfactoryEarlyAccess`。

## 如何查找 AOB

> [!CAUTION]
> 为游戏查找 AOB 并非易事，需要具备基础的逆向工程知识。这不是我们能写出面面俱到的指南的事情。

由于该过程相当复杂，这里只介绍你需要采取的一般步骤。

1. 在你的游戏所用的 UE 版本中，制作一个带 PDB 的空 Shipping 游戏
2. 使用 x64dbg 读取游戏内存
3. 查找你需要的特征码——这些特征码列在[下文](#每个-aob-的-onmatchfound-必须返回什么)中
4. 从该函数中抓取一段字节，有时函数头就足够了。如果不够，抓取对该函数的一次调用可能更好；如果它不是虚函数，你可以在那里抓取 RIP 地址
5. 在 x64dbg 中打开游戏内存，搜索相同的字节块
6. 如果找到了，可以用 [swiss army knife](https://github.com/Nukem9/SwissArmyKnife) 工具提取它的 AOB，然后用在简单脚本中，例如[这里的示例](#示例脚本简单直接扫描)

更详细的说明请参见[进阶指南](./fixing-compatibility-problems-advanced.md)。

## 如何设置自己的 AOB 与回调

1. 如果你的 `工作目录` 中还没有 `UE4SS_Signatures` 目录，就创建它。
2. 确定哪些 AOB 已失效并需要修复。
3. 根据失效的 AOB，在 `UE4SS_Signatures` 中创建以下文件：
   - GUObjectArray.lua
   - FName_ToString.lua
   - FName_Constructor.lua
   - StaticConstructObject.lua
   - GMalloc.lua
   - GUObjectHashTables.lua（可选）
   - GNatives.lua          （可选）
   - ConsoleManager.lua    （可选）
   - ProcessLocalScriptFunction.lua
   - ProcessInternal.lua
   - CallFunctionByNameWithArguments.lua
4. 在 `.lua` 文件中，你需要在文件作用域返回符号地址，或者需要一个 `Register` 函数与 `OnMatchFound` 函数来完成 AOB 扫描。
   - 请记住，`UE4SS_Signatures` 目录中 Lua 文件里的函数名是区分大小写的。
   - 如果你选择在文件作用域返回符号地址，`LoadExport` 函数可能有用，但通常只在游戏以模块化方式构建时才有用。
5. `Register` 函数必须返回你希望 UE4SS 扫描的 AOB。
   - 格式是一串半字节，每两个组成一个字节。
   - 为了清晰，我喜欢在每个字节之间加一个空格，但这不是必须的。
   - AOB 示例：`8B 51 04 85`。
   - 另一个 AOB 示例：`8B510485`。
   - AOB 扫描器支持对半字节或整个字节使用通配符。
6. 接下来你需要创建一个全局的 `OnMatchFound` 函数。
   - 该函数有一个参数 `MatchAddress`，即匹配到的地址。
   - 你要在这个函数中放置计算最终地址的全部逻辑。
   - 最简单的做法是确保你的 AOB 直接指向最终地址的开头，这样你只需返回 `MatchAddress`。
   - 如果你要做更复杂的事情（例如间接 AOB 扫描），UE4SS 提供了两个全局函数：`DerefToInt32`，它接收一个地址并以 32 位整数返回该处的数据，若无法解引用则返回 `nil`；以及用于调试的 `print`。

## 每个 AOB 的 'OnMatchFound' 必须返回什么

- GUObjectArray
  - 必须返回名为 'GUObjectArray' 的全局变量的确切地址。
- FName_ToString
  - 必须返回函数 'FName::ToString' 起始处确切地址。
    函数签名：`public: void cdecl FName::ToString(class FString & ptr64)const __ptr64`
- FName_Constructor
  - 必须返回函数 'FName::FName' 起始处确切地址。
    该回调很可能被调用很多次，我们在后台会做检查以确认是否找到了正确的构造函数。
    你的 AOB 同时匹配 'char*' 版本与 'wchar_t*' 版本都没有关系。
    函数签名：`public: cdecl FName::FName(wchar_t const * ptr64,enum EFindName) __ptr64`
- StaticConstructObject
  - 必须返回全局函数 'StaticConstructObject_Internal' 起始处确切地址。
    在 UE4SS 中，我们扫描 'UUserWidget::InitializeInputComponent' 中间的一处调用，然后解析该调用位置。
    函数签名：`class UObject * __ptr64 __cdecl StaticConstructObject_Internal(struct FStaticConstructObjectParameters const & __ptr64)`
- GMalloc
  - 必须返回名为 'GMalloc' 的全局变量的确切地址。
    在 UE4SS 中，我们扫描 `FMemory::Free`，然后解析距离第一条 CALL 指令最近的 MOV 指令。
- GNatives（可选）
  - 必须返回名为 'GNatives' 的全局变量的确切地址。
- GUObjectHashTables（开发中）（可选）
- ConsoleManager（开发中）（可选）
- ProcessLocalScriptFunction
  - 99% 的情况下都不需要。
  - 必须返回 ProcessLocalScriptFunction 的确切地址。
- ProcessInternal
  - 99% 的情况下都不需要。
  - 必须返回 ProcessInternal 的确切地址。
- CallFunctionByNameWithArguments
  - 99% 的情况下都不需要。
  - 必须返回 CallFunctionByNameWithArguments 的确切地址。

## 示例脚本（简单、直接扫描）

```lua
function Register()
    return "48 8B C4 57 48 83 EC 70 80 3D ?? ?? ?? ?? ?? 48 89"
end

function OnMatchFound(MatchAddress)
    return MatchAddress
end
```
