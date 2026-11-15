# UE4SS 中的 DataTable

## 背景

[DataTable](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/UDataTable/) 是虚幻引擎中的一种数据结构，允许在运行时加载哈希化的键值对。常见用途包括存放掉落表、升级所需经验值、actor 的基础生命值/护甲等……

DataTable 本是为在游戏编译期填充而设计的，*从技术上讲* 并不应该在运行时被修改。虚幻的文档有时与这一说法相矛盾，因此很难分清哪些是*设计意图*、哪些是*可能做到*的。我的目标是在不使用蓝图的情况下，从 C++ 上下文在运行时支持完整的读/写/更新/删除/遍历操作。

## 为什么不直接做一个替换 DataTable 的蓝图 mod？

这种做法*从技术上讲*可行。问题在于，你的 mod 是**唯一**能修改该 DataTable 的 mod。对于希望同时使用多个都要修改同一 DataTable 的 mod 的客户端来说，这显然不理想。从可扩展性角度我给这个方案打大约 2/10 分。

## DataTable 的结构是什么？

DataTable 由原生虚幻的 `TMap` 与 `TSet` 构建。如果你熟悉 Java 的 `HashMap` 或 C# 的 `Dictionary`，那么你就大致明白其约定与用法。虚幻的 `DataTable` 键为 [`FName`](https://docs.unrealengine.com/4.26/en-US/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/StringHandling/FName/)，值为一个继承自 [`FTableRowBase`](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/FTableRowBase/) 的 struct。稍后会详细讨论……

## 那么需要做什么？

我会列出几种修改 DataTable 的可能方案，并评估每种方案的可行性与稳定性，以便提供一些参考。

### 方案 1（TMap 实现）

虚幻引擎中的 DataTable 暴露了一个可访问的 RowMap 属性：

```cpp
// DataTable.h
virtual const TMap< FName, uint8 * > & GetRowMap() const 
virtual const TMap< FName, uint8 * > & GetRowMap()  
```

`GetRowMap()` 函数是已反射的，使用 UVTD 文件可以很容易地调用它。问题在于 UE4SS 的 TMap 实现非常简陋。目前 UE4SS 的 TMap 实现可以这样使用：

```cpp
// DataTable 行格式为 <FName, CoolStruct>
struct CoolStruct : FTableRowBase
{
    FString SomeString;
    int_32 SomeNumber;
    bool SomeBoolean;
}

TMap<FName, unsigned char*> rowMap = dataTable->GetRowMap();
auto ptrElem = rowMap.GetElementsPtr();
for(int32_t i = 0; i < rowMap.Num(); i++)
{
    auto pair = &ptrElem[i];
    pair->Key;
    pair->Value;
    CoolStruct* row = reinterpret_cast<CoolStruct*>(pair->Value);
}
```

**那有什么大问题？**

UE4SS 的 TMap 不喜欢底层数据被修改。这种访问方式对 DataTable 的读取/遍历还算不错，但一旦我们调用 `dt->AddRow()` 或 `dt->RemoveRow()`，底层的 `.GetElementsPtr()` 就不再准确。如果你看过 UE4SS 中 TMap 的实现，就会发现它相当脆弱，除非你只打算读取。

注意，UE4SS TMap 中当前的 `.Num()` 函数并不会真正对 TMap 做计算。`Num` 属性只是在我们于 UE4SS 中构造 TMap 时设置的，因此底层大小变化时我们无法获得更新。

我认为如果你只是想读取 DataTable，这个方案是合理的。

那么怎样才能让它真正可用？

理论上我们可以在 UE4SS 中实现一个与 UE 原生行为对应的 TMap。UE4SS 对 `TArray` 就采取了类似做法。潜在缺点是：如果 TMap 的底层逻辑/结构在不同 UE 版本之间发生变化，我们就需要多个实现来代表不同版本下 UE TMap 的状态。或者，我们可以使用 `#if UE5_1` 之类的条件编译，把内容都集中在一个 TMap.hpp/cpp 文件中。

在 UE4SS 中实现 TMap 能否真正修改 DataTable？我还没有做彻底调查，但直觉告诉我……*大概可以？*

**为什么不能在 DataTable 对象上使用 FindRow/GetRow？**

我们从 `UDataTable` 转储中得到的有用反射函数只有 `GetRowMap()`、`RemoveRow()` 和 `AddRow()`。这不算太差，但遗憾的是我们无法直接获取某一行，也无法用 UE4SS 的 `TMap` 获取某一行。

### 方案 2（Kismet DataTable 辅助库）

该方案利用虚幻引擎内置的蓝图 [DataTable 辅助类](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Kismet/UDataTableFunctionLibrary/)。该蓝图辅助类中已反射的函数有：

```cpp
static bool DoesDataTableRowExist
(
    UDataTable * Table,
    FName RowName
) 

static void GetDataTableRowNames
(
    UDataTable * Table,
    TArray< FName > & OutRowNames
) 

static bool GetDataTableRowFromName
(
    UDataTable * Table,
    FName RowName,
    FTableRowBase & OutRow
) 
```

如果你一直在认真阅读，可能已经灵光一闪。似乎我们可以通过组合以下函数来实现完整的 DataTable 支持：

```cpp
// DataTable 已反射函数
AddRow();
RemoveRow();
Empty();

// DataTableFunctionLibrary 已反射函数
DoesDataTableRowExist();
GetDataTableRowNames();
GetDataTableRowFromName();
```

**但总有意外……**

`GetDataTableRowFromName();` 是一个尤其棘手的函数。简而言之，它*大概*可用，但还需要进一步实验。

下一节需要你对 Kismet/蓝图/FFrame 以及蓝图脚本栈的工作方式有较深入的了解。我会附上一些预备阅读材料。

* [Custom Thunks 简介](https://gist.github.com/intaxwashere/e9b1f798427686b46beab2521d7efbcf)
* [从 C++ 调用蓝图](https://intaxwashere.github.io/blueprint-access/)
* [蓝图函数模板](https://neil3d.github.io/unreal/blueprint-wildcard.html)

`GetDataTableRowFromName()` 带有 [说明符](https://docs.unrealengine.com/4.26/en-US/ProgrammingAndScripting/GameplayArchitecture/Functions/Specifiers/) `CustomThunk` 与 `CustomStructureParam`。

```
CustomThunk:
UnrealHeaderTool 代码生成器不会为该函数生成 thunk；用户需要用 DECLARE_FUNCTION 或 DEFINE_FUNCTION 宏自行提供。

CustomStructureParam:
列出的参数全部按通配符处理。该说明符需要 UFUNCTION 级别的说明符 CustomThunk，后者要求用户提供自定义 exec 函数。在该函数中，可以检查参数类型，并基于这些类型调用合适的函数。基础 UFUNCTION 绝不应被调用，被调用时应当断言或记录错误。
```

在底层，`GetDataTableRowFromName()` 这个 UFunction 只是一个桩。DataTableFunctionLibrary 通过 `DEFINE_FUNCTION(execGetDataTableRowFromName)` 宏提供真正的行为。我们来看看定义出来的函数：

```cpp
// DataTableFunctionLibrary.h
 /** Based on UDataTableFunctionLibrary::GetDataTableRow */
 DECLARE_FUNCTION(execGetDataTableRowFromName)
 {
     P_GET_OBJECT(UDataTable, Table);
     P_GET_PROPERTY(FNameProperty, RowName);
     
     Stack.StepCompiledIn<FStructProperty>(NULL);
     void* OutRowPtr = Stack.MostRecentPropertyAddress;

		P_FINISH;
		bool bSuccess = false;
		// 下面这一行无法找到 StructProp。具体原因见本代码块之后的说明。
		FStructProperty* StructProp = CastField<FStructProperty>(Stack.MostRecentProperty);
		if (!Table)
		{
FBlueprintExceptionInfo ExceptionInfo(
	EBlueprintExceptionType::AccessViolation,
	NSLOCTEXT("GetDataTableRow", "MissingTableInput", "Failed to resolve the table input. Be sure the DataTable is valid.")
);
FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		}
		else if(StructProp && OutRowPtr)
		{
UScriptStruct* OutputType = StructProp->Struct;
const UScriptStruct* TableType  = Table->GetRowStruct();
		
const bool bCompatible = (OutputType == TableType) || 
	(OutputType->IsChildOf(TableType) && FStructUtils::TheSameLayout(OutputType, TableType));
if (bCompatible)
{
	P_NATIVE_BEGIN;
	bSuccess = Generic_GetDataTableRowFromName(Table, RowName, OutRowPtr);
	P_NATIVE_END;
}
else
{
	FBlueprintExceptionInfo ExceptionInfo(
		EBlueprintExceptionType::AccessViolation,
		NSLOCTEXT("GetDataTableRow", "IncompatibleProperty", "Incompatible output parameter; the data table's type is not the same as the return type.")
		);
	FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
}
		}
		else
		{
FBlueprintExceptionInfo ExceptionInfo(
	EBlueprintExceptionType::AccessViolation,
	NSLOCTEXT("GetDataTableRow", "MissingOutputProperty", "Failed to resolve the output parameter for GetDataTableRow.")
);
FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		}
		*(bool*)RESULT_PARAM = bSuccess;
 }
```

问题在于，当我们从 C++ 上下文调用 `GetDataTableRowFromName()` 时，`Stack.MostRecentProperty` 不会被填充。这一点在以下 GitHub issue 中有记录：

* [Issue 1](https://github.com/Tencent/puerts/issues/836)（中文）
* [Issue 2](https://github.com/Tencent/puerts/issues/986)（中文）

在底层：

```cpp
static bool GetDataTableRowFromName
(
    UDataTable * Table,
    FName RowName,
    FTableRowBase & OutRow
)

// 进行一些属性读取、类型检查等操作，
// 然后在内部调用

static bool Generic_GetDataTableRowFromName
(
    const UDataTable * Table,
    FName RowName,
    void * OutRowPtr
) 
```

对我们来说，用 `void*` 代替 `FTableRowBase&` 来作为 `OutRow` 会更合适，但遗憾的是，`Generic_GetDataTableRowFromName()` 无法通过反射访问。

问题的核心在于，`execGetDataTableRowFromName()` 在类型检查方面特别激进，会确保函数要么正常工作，要么优雅退出。这可以理解，因为该函数是一个蓝图节点，需要在蓝图框架内稳健运行。`Stack.MostRecentProperty` 的具体用途是确定我们期望从 DataTable 中取出的 Struct 目标类型。在蓝图调用方上下文中，该属性会作为 Kismet FFrame/Stack 流程的一部分被填充。

**我们能做点什么吗？**

我目前正在尝试手动设置 `Stack.MostRecentProperty`，以骗过 `GetDataTableRowFromName()`，让它*以为*我们是在作为合法的蓝图函数调用该函数，而不是直接从 C++ 代码调用。和方案 1 一样，在功能层面我给它评为*大概可以？*

### 最后一个变数……

关于 DataTable 行结构体在内存中如何存储，还需要进一步研究。看起来某些游戏可能存在编译器打包，但范围仍不清楚。此外，有些游戏出于内存占用/对齐/填充的考虑把结构体成员排布得相当合理，而另一些游戏的结构体成员布局从可读性角度说得通，但并不利于内存优化。

```cpp
// NameTypes.hpp（UE4SS）

// TODO:   搞清楚这里发生了什么
//         不应该需要用 'alignas' 来确保它在容器（如 TArray）中正确对齐
//         我从没见过内存中未按 8 字节对齐的 FName，
//         但源码中它是 4 字节对齐的，希望这不会造成问题
// UPDATE: 这在 UE VM 中很重要，当保留大小写（case-preserving）的游戏内存中 ElementSize 为 0xC 时，此时必须按 0x4 对齐
#pragma warning(disable: 4324) // 抑制关于结构体对齐的警告
#ifdef WITH_CASE_PRESERVING_NAME
    struct alignas(4) RC_UE_API FName
#else
    struct alignas(8) RC_UE_API FName // DataTable 行中的 FName 似乎只在 alignas(4) 时正常工作
```

上面的代码是一个 TODO：它仍然留在 UE4SS 中。对对齐问题的研究很可能也会对其他非 DataTable 的部分有帮助！无论我们选择哪个方案（TMap、蓝图库或其他），都需要弄清对齐/填充的完整情况。

## 免责声明

虽然我相信自己已较好地理解了相关因素，但我毫不怀疑自己遗漏了一些细微之处，并且对底层系统的某些部分理解有误。如果你认为某些行为与当前文档描述不同，请告诉我。我会非常感激你的帮助！

## 有想法？

请在 UE4SS Discord 中联系我，一起讨论/分享你的想法。虽然我目前担任 DataTable 的功能负责人，但我非常感谢一切帮助。

## 其他资源

* [DataTable Pull Request](https://github.com/Re-UE4SS/UEPseudo/pull/74/commits/04ef456ea337c59bc2df8ba9162ce4a7e52de445) - 查看它似乎需要 Epic Games 组权限？
* [UE5 Wiki](https://ue5wiki.com/wiki/34184/)（中文）
* [UE4SS 文档](https://docs.ue4ss.com/)
* [JIP 博客](https://jip.dev/notes/unreal-engine/)

## 致谢

特别感谢 localcc 作为出色的导师。也向所有 DataTable 分支的早期使用者致敬（特别感谢 El 成为我们第一位早期使用者）。

感谢你一直以来的耐心。
