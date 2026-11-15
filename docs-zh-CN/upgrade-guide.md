# RE-UE4SS 升级指南

本文档提供在 RE-UE4SS 各版本之间升级的详细指导。每一节介绍一次具体的版本升级，说明破坏性变更、弃用内容以及成功迁移代码所需的步骤。

## 从 3.x 升级到 4.x

> **发布日期：** [待定]
> **PR：** [可用时补充链接]

### 破坏性变更

#### FName 构造函数默认参数变化

**变更内容：**
FName 构造函数的默认 `EFindName` 参数已从 `FNAME_Find` 改为 `FNAME_Add`。

**变更前（v3.x）：**

```cpp
// 默认是 FNAME_Find —— 只查找已存在的名称
explicit FName(const CharType* StrName, EFindName FindType = FNAME_Find, void* FunctionAddressOverride = nullptr);
```

**变更后（v4.x）：**

```cpp
// 默认现在是 FNAME_Add —— 名称不存在时创建它
explicit FName(const CharType* StrName, EFindName FindType = FNAME_Add, void* FunctionAddressOverride = nullptr);
```

**迁移步骤：**

1. 如果你依赖 FName 构造函数在名称不存在时返回 NAME_None 的旧行为：

   ```cpp
   // 旧写法（隐式 FNAME_Find）
   FName TestName(STR("NonExistentName")); // 若名称不存在会得到 NAME_None

   // 新写法（显式使用 FNAME_Find 以保持旧行为）
   FName TestName(STR("NonExistentName"), FNAME_Find); // 显式指定 FNAME_Find
   ```

2. 如果你在构造之后检查名称是否有效：

   ```cpp
   // 可能失效的旧代码
   FName TestName(STR("PotentiallyNewName"));
   if (!TestName) { // 该检查不再按预期工作
       // 处理无效名称
   }

   // 新做法
   FName TestName(STR("PotentiallyNewName"), FNAME_Find); // 需要检查是否存在时使用 FNAME_Find
   if (!TestName) {
       // 处理不存在的名称
   }
   ```

3. 对于已经显式指定 FindType 参数的代码，无需改动。

**重要说明：**

- 该变更影响所有接受字符串参数的三个 FName 构造函数重载
- 新的默认行为（`FNAME_Add`）会在名称表中创建不存在的 FName 条目
- 如果你频繁用可能不存在的字符串创建 FName，这可能带来性能影响
- 与虚幻引擎内部打交道时要小心，不小心创建新名称可能导致意外行为

#### TObjectPtr 实现变化

**变更内容：**
`TObjectPtr<>` 类已增强为真正的智能指针，而不是简单的包装器。

**变更前（v3.x）：**

```cpp
// 一个让一切都能编译的简单类。
template<typename UnderlyingType>
class TObjectPtr
{
public:
    UnderlyingType* UnderlyingObjectPointer;
};
```

**变更后（v4.x）：**

```cpp
template<typename T>
class TObjectPtr
{
public:
    using ElementType = T;
    // 构造函数
    TObjectPtr() : Ptr(nullptr) {}
    TObjectPtr(TYPE_OF_NULLPTR) : Ptr(nullptr) {}
    TObjectPtr(const TObjectPtr& Other) : Ptr(Other.Ptr) {}
    explicit TObjectPtr(ElementType* InPtr) : Ptr(InPtr) {}

    // 赋值运算符
    TObjectPtr& operator=(const TObjectPtr& Other) { Ptr = Other.Ptr; return *this; }
    TObjectPtr& operator=(TYPE_OF_NULLPTR) { Ptr = nullptr; return *this; }
    TObjectPtr& operator=(ElementType* InPtr) { Ptr = InPtr; return *this; }

    // 转换运算符
    FORCEINLINE operator T* () const { return Get(); }
    template <typename U>
    UE_OBJPTR_DEPRECATED(5.0, "Explicit cast to other raw pointer types is deprecated.  Please use the Cast API or get the raw pointer with ToRawPtr and cast that instead.")
    explicit FORCEINLINE operator U* () const { return (U*)Get(); }
    explicit FORCEINLINE operator UPTRINT() const { return (UPTRINT)Get(); }
    FORCEINLINE T* operator->() const { return Get(); }
    FORCEINLINE T& operator*() const { return *Get(); }

    // 比较运算符
    bool operator==(const TObjectPtr& Other) const { return Ptr == Other.Ptr; }
    bool operator!=(const TObjectPtr& Other) const { return Ptr != Other.Ptr; }
    bool operator==(const ElementType* InPtr) const { return Ptr == InPtr; }
    bool operator!=(const ElementType* InPtr) const { return Ptr != InPtr; }
    bool operator==(TYPE_OF_NULLPTR) const { return Ptr == nullptr; }
    bool operator!=(TYPE_OF_NULLPTR) const { return Ptr != nullptr; }

    // 与 UE 的 TObjectPtr 的额外 API 兼容性
    bool operator!() const { return Ptr == nullptr; }
    explicit operator bool() const { return Ptr != nullptr; }
    ElementType* Get() const { return Ptr; }

private:
    ElementType* Ptr;
};
```

**迁移步骤：**

1. 直接访问指针（以前通过 `UnderlyingObjectPointer` 访问）：

   ```cpp
   // 旧写法
   TObjectPtr<UClass> ClassPtr;
   UClass* RawPtr = ClassPtr.UnderlyingObjectPointer;

   // 新写法
   TObjectPtr<UClass> ClassPtr;
   UClass* RawPtr = ClassPtr; // 隐式转换
   // 或
   UClass* RawPtr = ClassPtr.Get(); // 显式访问
   ```

2. 对 TObjectPtr 内容取地址：

   ```cpp
   // 旧写法
   &ObjectPtr.UnderlyingObjectPointer

   // 新写法
   &ObjectPtr.Get()
   // 或直接写
   &ObjectPtr
   ```

3. 指针与整数之间的转换（例如格式化地址）：

   ```cpp
   // 旧写法
   uintptr_t addr = reinterpret_cast<uintptr_t>(ObjectPtr.UnderlyingObjectPointer);

   // 新写法
   uintptr_t addr = reinterpret_cast<uintptr_t>(ObjectPtr.Get());
   // 或
   uintptr_t addr = reinterpret_cast<uintptr_t>(ToRawPtr(ObjectPtr)); // 使用辅助函数从 TObjectPtr 取出原始指针
   ```

#### FString API 变化

##### `GetCharArray()` 行为变化

**变更内容：**
`GetCharArray()` 方法现在返回实际的数组，而不是指向字符的指针。

**变更前（v3.x）：**

```cpp
// 返回字符指针
auto GetCharArray() const -> const CharType*;
```

**变更后（v4.x）：**

```cpp
// 返回数组本身
FORCEINLINE DataType& GetCharArray() { return Data; }
FORCEINLINE const DataType& GetCharArray() const { return Data; }

// 字符指针访问请使用 operator*
FORCEINLINE const TCHAR* operator*() const { return Data.Num() ? Data.GetData() : TEXT(""); }
```

**迁移步骤：**

1. 字符指针访问（以前用 `GetCharArray()`）：

   ```cpp
   // 旧写法
   const TCHAR* ptr = myString.GetCharArray();

   // 新写法
   const TCHAR* ptr = *myString;
   ```

2. 数组访问（以前用 `GetCharTArray()`）：

   ```cpp
   // 旧写法
   TArray<TCHAR>& arr = myString.GetCharTArray();

   // 新写法
   TArray<TCHAR>& arr = myString.GetCharArray();
   ```

#### FSoftObjectPath 成员变量改为访问器

**变更内容：**
`FSoftObjectPath` 不再暴露 `AssetPathName` 与 `SubPathString` 成员变量。该结构体现在存储不透明的字节，并在运行时解析引擎的真实布局，因为 UE 5.1 改变了布局（`FName AssetPathName` 变成了 `FTopLevelAssetPath AssetPath`，`SubPathString` 的偏移从 0x8 移到 0x10）。旧的编译期成员在所有 5.1+ 游戏上都会读取错误的内存。

**变更前（v3.x）：**

```cpp
FSoftObjectPath Path = ...;
FName AssetName = Path.AssetPathName;
FString& SubPath = Path.SubPathString;
Path.SubPathString = NewSubPath;
```

**变更后（v4.x）：**

```cpp
FSoftObjectPath Path = ...;
FName AssetName = Path.GetAssetPathName();
FString& SubPath = Path.GetSubPathString();
Path.GetSubPathString() = NewSubPath;

// 新增：包名/资源名对，在所有引擎版本上都可用
FTopLevelAssetPath AssetPath = Path.GetAssetPath();
```

**迁移步骤：**

1. 把 `Path.AssetPathName` 替换为 `Path.GetAssetPathName()`。
2. 把 `Path.SubPathString` 替换为 `Path.GetSubPathString()`。存在非 const 重载，因此读取、写入与按引用传递都可以正常工作。
3. 如果你需要分别获取包名与资源名，优先使用 `GetAssetPath()`，它在所有引擎版本中都返回 `FTopLevelAssetPath`（在 5.1 之前的版本上通过拆分路径合成）。

**重要说明：**

- 在 5.1+ 引擎上没有单一 FName 表示，因此 `GetAssetPathName()` 会按需创建合并后的 `/Package/Path.Asset` FName（使用 `FNAME_Add`）。
- 写入资源路径通过 `SetPath()` 或 `operator=(const FString&)`，与以前相同。
- `TArray<FSoftObjectPath>` 现在按运行时大小（`FSoftObjectPath::StaticSize()`）步进，修复了 5.1+ 以及保留大小写构建中的元素访问问题。
- Lua API（`GetAssetPathName()`、`GetSubPathString()`）没有变化，Lua mod 不受影响。

#### 软对象指针成员变量改为访问器

**变更内容：**

`FSoftObjectPtr` 及其基类 `TPersistentObjectPtr<FSoftObjectPath>` 不再把 `ObjectID` 与 `TagAtLastTest` 作为成员变量暴露。它们的布局根据引擎反射出的软对象/类属性大小解析，当反射不可用时回退到引擎版本。这同样适用于 `TSoftObjectPtr<T>` 与 `TSoftClassPtr<T>` 内部的存储。

UE 5.3 移除了 `TagAtLastTest`，把 `ObjectID` 的偏移从 `0x10` 移到 `0x08`。这与上面所述的 UE 5.1 `FSoftObjectPath` 变化是两回事。在旧偏移处读取路径，可能会把字符串长度与容量解释为指针，从而引发 [issue #1433](https://github.com/UE4SS-RE/RE-UE4SS/issues/1433) 中报告的崩溃：`FUObjectArray::AllocateSerialNumber` 在把对象转换为软引用时出错。

**变更前（v3.x / 更早的实验性构建）：**

```cpp
FSoftObjectPtr SoftPtr;
FSoftObjectPath& Path = SoftPtr.ObjectID;
int32 Tag = SoftPtr.TagAtLastTest;
SoftPtr.TagAtLastTest = 0;
```

**变更后（v4.x）：**

```cpp
FSoftObjectPtr SoftPtr;
FSoftObjectPath& Path = SoftPtr.GetUniqueID();
int32 Tag = SoftPtr.GetTagAtLastTest();
SoftPtr.SetTagAtLastTest(0);
```

**迁移步骤：**

1. 把直接访问 `ObjectID` 改为 `GetUniqueID()`。const 与非 const 重载都返回引用。
2. 读取标记改用 `GetTagAtLastTest()`，写入改用 `SetTagAtLastTest(value)`。
3. 替换路径时，优先使用 `SoftPtr = NewPath`，它还会重置缓存的弱指针。如果你通过非 const 的 `GetUniqueID()` 引用修改路径，之后请调用 `ResetWeakPtr()`。
4. 手动遍历引擎拥有的内存时，请使用 `FSoftObjectPtr::StaticSize()`、`TSoftObjectPtr<T>::StaticSize()` 或 `TSoftClassPtr<T>::StaticSize()`，而不是 `sizeof` 或硬编码的步长。`sizeof` 包含支持的最大布局所需的存储，仍然是本地分配的 C++ 对象应使用的值。
5. 针对更新后的 UE4SS 头文件与库重新构建受影响的 C++ mod。

**重要说明：**

- 在没有该标记字段的引擎上，`GetTagAtLastTest()` 返回零，`SetTagAtLastTest()` 不起作用。
- `WeakPtr` 仍然可以访问。`TSoftObjectPtr<T>` 与 `TSoftClassPtr<T>` 的既有方法（例如 `Get()`、`Reset()` 与 `ToSoftObjectPath()`）仍然可用。
- 请通过软指针的 C++ 操作来复制与赋值，以便正确复制路径的 `FString`；按字节复制并不会建立字符串的所有权。
- Lua 的 `GetWeakPtr()`、`GetObjectID()` 与 `GetTagAtLastTest()` 方法保持不变。Lua mod 无需迁移；当引擎没有标记字段时 `GetTagAtLastTest()` 返回零。

#### FText 成员变量改为访问器

**变更内容：**
`FText` 不再暴露 `Data`、`SharedRefCollector`、`Flags` 与 `Unk` 成员变量。UE 5.4 把内部表示从 `TSharedRef<ITextData>`（FText 为 0x18 字节）改为 `TRefCountPtr<ITextData>`（0x10 字节），因此固定的成员布局不可能在所有版本上都正确。FText 现在存储由 `FText::StaticSize()` 确定大小的不透明字节，复制、赋值与销毁都经由引擎自身的 `FTextProperty` 值操作，这意味着在所有引擎版本上 FText 的复制都会被正确引用计数。

**变更前（v3.x）：**

```cpp
FText Text = ...;
ITextData* Data = Text.Data;
uint32 Flags = Text.Flags;
```

**变更后（v4.x）：**

```cpp
FText Text = ...;
ITextData* Data = Text.GetTextData();
uint32 Flags = Text.GetFlags();
```

**迁移步骤：**

1. 把 `Text.Data` 替换为 `Text.GetTextData()`。
2. 把 `Text.Flags` 替换为 `Text.GetFlags()`。
3. 删除任何对 `SharedRefCollector` 或 `Unk` 的使用；它们只是猜测布局的产物。引用控制器现在由引擎管理。
4. 构造、复制、赋值或转换 FText 的代码（`ToString()`、`ToFString()`、`SetString()`、`FText(FString)`）无需改动。

**重要说明：**

- 通过 C++ 完成的复制现在是拥有型引用，FText 销毁时会自动释放。按字节复制 FText 值（memcpy 到比源对象存活更久的缓冲区）应改为赋值，或在处理引擎参数缓冲区时使用 `FText::StealFromBuffer` / `CopyBorrowedTo`。
- 按值把 FText 传给引擎函数仍然正常工作；临时对象自带引用。
- Lua API 没有变化，Lua mod 不受影响。

### 弃用

- `GetCharTArray()` 现已弃用，但为兼容性仍然可用。
  它会转发到 `GetCharArray()`，并将在未来版本中移除。

#### 属性头文件合并

**变更内容：**
单独的属性头文件（`Unreal/Property/*.hpp` 与 `Unreal/FProperty.hpp`）已弃用。几乎所有属性类型现在都合并到 `Unreal/CoreUObject/UObject/UnrealType.hpp`。

**变更前（v3.x）：**

```cpp
#include <Unreal/FProperty.hpp>
#include <Unreal/Property/FArrayProperty.hpp>
#include <Unreal/Property/FObjectProperty.hpp>
#include <Unreal/Property/FStructProperty.hpp>
// 等等
```

**变更后（v4.x）：**

```cpp
// 所有属性类型只需一个 include
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>
```

**迁移步骤：**

1. 把以下任何与属性相关的 include 替换为 `Unreal/CoreUObject/UObject/UnrealType.hpp`：

   ```cpp
   // 移除这些已弃用的 include：
   // #include <Unreal/FProperty.hpp>
   // #include <Unreal/Property/FArrayProperty.hpp>
   // #include <Unreal/Property/FBoolProperty.hpp>
   // #include <Unreal/Property/FClassProperty.hpp>
   // #include <Unreal/Property/FDelegateProperty.hpp>
   // #include <Unreal/Property/FInterfaceProperty.hpp>
   // #include <Unreal/Property/FLazyObjectProperty.hpp>
   // #include <Unreal/Property/FMapProperty.hpp>
   // #include <Unreal/Property/FMulticastDelegateProperty.hpp>
   // #include <Unreal/Property/FMulticastInlineDelegateProperty.hpp>
   // #include <Unreal/Property/FMulticastSparseDelegateProperty.hpp>
   // #include <Unreal/Property/FNameProperty.hpp>
   // #include <Unreal/Property/FNumericProperty.hpp>
   // #include <Unreal/Property/FObjectProperty.hpp>
   // #include <Unreal/Property/FSetProperty.hpp>
   // #include <Unreal/Property/FSoftClassProperty.hpp>
   // #include <Unreal/Property/FSoftObjectProperty.hpp>
   // #include <Unreal/Property/FStructProperty.hpp>
   // #include <Unreal/Property/FWeakObjectProperty.hpp>
   // #include <Unreal/Property/NumericPropertyTypes.hpp>

   // 替换为：
   #include <Unreal/CoreUObject/UObject/UnrealType.hpp>
   ```

2. 无需改动代码——所有属性类型仍然以相同的名称与 API 提供。

**为什么这样改：**

- 简化 include 依赖
- 缩短编译时间
- 与虚幻引擎自身的头文件组织方式一致（几乎所有属性类型都定义在 `UnrealType.h` 中）
- 消除 include 顺序问题

#### 属性遍历 API 改进

**变更内容：**
现有的 `ForEachProperty()` 与 `ForEachPropertyInChain()` 方法仍然可用并完全受支持，但我们现在建议直接使用 Epic 的 `TFieldIterator<>` 与 `TFieldRange<>` 模式，以便与虚幻引擎代码保持一致并获得更清晰的遍历控制。

**现有 API（仍受支持）：**

```cpp
// 基于迭代器的遍历（仍然可用）
for (FProperty* Property : MyStruct->ForEachProperty())
{
    // 处理属性
}

for (FProperty* Property : MyStruct->ForEachPropertyInChain())
{
    // 处理属性（含父类）
}
```

**推荐 API（v4.x 新增）：**

```cpp
// TFieldIterator —— Epic 的子类到父类遍历模式
#include <Unreal/UnrealType.hpp>

// 只遍历本类中的属性（不含父类）
for (FProperty* Property : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::None))
{
    // 处理属性
}

// 遍历所有属性，含父类（子类到父类顺序）
for (FProperty* Property : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::IncludeSuper))
{
    // 处理属性，包括继承链
}

// 包含已弃用的属性
for (FProperty* Property : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::IncludeDeprecated))
{
    // 处理所有属性，包括已弃用的
}

// 组合标志
for (FProperty* Property : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::IncludeSuper | EFieldIterationFlags::IncludeDeprecated))
{
    // 处理继承链上的所有属性，包括已弃用的
}

// 按父类到子类顺序遍历（反向）
for (FProperty* Property : TReverseFieldRange<FProperty>(MyStruct, EFieldIterationFlags::None))
{
    // 从父类到子类处理属性
}

// 也可以遍历其他字段类型
for (UFunction* Function : TFieldRange<UFunction>(MyClass, EFieldIterationFlags::IncludeSuper))
{
    // 处理函数
}
```

**EFieldIterationFlags 选项：**

```cpp
enum EFieldIterationFlags : uint8
{
    None                 = 0,      // 只遍历该结构体/类中的字段
    IncludeSuper        = 1 << 0,  // 包含父类继承的字段
    IncludeDeprecated   = 1 << 1,  // 包含已弃用的属性
    IncludeInterfaces   = 1 << 2,  // 包含接口属性（仅类）

    // 便捷别名
    Default = IncludeSuper,        // Epic 的默认行为
};
```

**迁移示例：**

1. 简单的属性遍历：

   ```cpp
   // 旧写法（仍然可用）
   for (FProperty* Prop : MyStruct->ForEachProperty())
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }

   // 新写法（推荐）
   for (FProperty* Prop : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::None))
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }
   ```

2. 含继承的遍历：

   ```cpp
   // 旧写法（仍然可用）
   for (FProperty* Prop : MyStruct->ForEachPropertyInChain())
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }

   // 新写法（推荐）
   for (FProperty* Prop : TFieldRange<FProperty>(MyStruct, EFieldIterationFlags::IncludeSuper))
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }
   ```

3. 父类到子类的遍历顺序：

   ```cpp
   // 旧写法（仍然可用）
   for (FProperty* Prop : OrderedForEachPropertyInChain(MyStruct))
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }

   // 新写法（推荐）
   for (FProperty* Prop : TReverseFieldRange<FProperty>(MyStruct, EFieldIterationFlags::None))
   {
       Output::send(STR("{}\n"), Prop->GetName());
   }
   ```

4. 直接使用迭代器：

   ```cpp
   // 使用显式迭代器获得更多控制
   for (TFieldIterator<FProperty> It(MyStruct, EFieldIterationFlags::IncludeSuper); It; ++It)
   {
       FProperty* Prop = *It;
       Output::send(STR("{}\n"), Prop->GetName());
   }
   ```

**为什么考虑使用新 API：**

- **一致性：** 完全匹配 Epic 的 UE4/UE5 代码库模式
- **灵活性：** 可细粒度控制包含内容（父类、已弃用属性、接口）
- **类型安全：** 可以遍历任意字段类型（`FProperty`、`UFunction`、`UEnum` 等）
- **熟悉度：** 如果你熟悉 Epic 的 API，那么这个模式你已经会了
- **明确性：** 在调用处即可看清遍历行为

**重要说明：**

- 旧的 `ForEachProperty()` 方法**已弃用**，未来可能被移除
- 为获得更好性能并与 Epic 的模式一致，请使用 `TFieldIterator`/`TFieldRange`

### 弃用

#### ExecuteAsync 与 LoopAsync

**变更内容：**
`ExecuteAsync` 与 `LoopAsync` 现已弃用，改用新的延迟操作（Delayed Action）系统。这些函数虽然仍然可用，但缺少新系统的控制与安全特性。

**为什么迁移：**

- **取消：** 旧函数一旦启动就无法取消
- **暂停/恢复：** 无法暂停异步循环
- **查询状态：** 无法检查操作是否待执行或获取剩余时间
- **Mod 隔离：** 旧函数没有归属跟踪
- **线程安全：** 新系统具有更好的线程安全性，尤其是在与游戏对象交互时

**迁移指南：**

| 旧函数 | 新函数 |
|--------|--------|
| `ExecuteAsync(delayMs, callback)` | `ExecuteInGameThreadWithDelay(delayMs, callback)` |
| `LoopAsync(delayMs, callback)` | `LoopInGameThreadWithDelay(delayMs, callback)` |

```lua
-- 旧写法（已弃用）
ExecuteAsync(1000, function()
    print("After 1 second\n")
end)

LoopAsync(1000, function()
    print("Every second\n")
    return false  -- 返回 true 停止
end)

-- 新写法（推荐）
ExecuteInGameThreadWithDelay(1000, function()
    print("After 1 second\n")
end)

local loopHandle
loopHandle = LoopInGameThreadWithDelay(1000, function()
    print("Every second\n")
    if shouldStop then
        CancelDelayedAction(loopHandle)
    end
end)
```

**迁移收益：**

- 随时可用 `CancelDelayedAction(handle)` 取消循环
- 通过 `PauseDelayedAction(handle)` 与 `UnpauseDelayedAction(handle)` 暂停/恢复
- 通过 `IsDelayedActionActive(handle)`、`GetDelayedActionTimeRemaining(handle)` 等查询状态
- 使用 `ClearAllDelayedActions()` 一次性清理你 mod 的所有计时器

### 新功能

#### 延迟操作（Delayed Action）系统

**新增内容：**
Lua API 中加入了完整的计时器/延迟操作（Delayed Action）系统。它提供 UE 风格的延迟执行，并完全控制计时、暂停、取消与循环。**这是 `ExecuteAsync` 与 `LoopAsync` 的推荐替代方案。**

**新增函数：**

- `ExecuteInGameThreadWithDelay(delayMs, callback)` - 延迟后执行回调，返回句柄
- `ExecuteInGameThreadWithDelay(handle, delayMs, callback)` - 仅当句柄未激活时执行（UE Delay 风格）
- `RetriggerableExecuteInGameThreadWithDelay(handle, delayMs, callback)` - 再次调用时重置计时器
- `LoopInGameThreadWithDelay(delayMs, callback)` - 重复计时器
- `ExecuteInGameThreadAfterFrames(frames, callback)` - 基于帧的延迟
- `LoopInGameThreadAfterFrames(frames, callback)` - 基于帧的重复计时器
- `MakeActionHandle()` - 生成用于延迟函数的唯一句柄

**计时器控制函数：**

- `CancelDelayedAction(handle)` - 取消待执行的操作
- `PauseDelayedAction(handle)` - 暂停计时器
- `UnpauseDelayedAction(handle)` - 恢复已暂停的计时器
- `ResetDelayedActionTimer(handle)` - 以原延迟重新启动
- `SetDelayedActionTimer(handle, newDelayMs)` - 修改延迟并重新启动
- `ClearAllDelayedActions()` - 取消当前 mod 的所有操作

**查询函数：**

- `IsValidDelayedActionHandle(handle)` - 检查句柄是否存在
- `IsDelayedActionActive(handle)` - 检查计时器是否运行中
- `IsDelayedActionPaused(handle)` - 检查计时器是否已暂停
- `GetDelayedActionRate(handle)` - 获取配置的延迟
- `GetDelayedActionTimeRemaining(handle)` - 获取剩余时间
- `GetDelayedActionTimeElapsed(handle)` - 获取已用时间

**新增全局变量：**

- `EngineTickAvailable` - 布尔值，表示 EngineTick 钩子是否可用
- `ProcessEventAvailable` - 布尔值，表示 ProcessEvent 钩子是否可用

**新增枚举：**

- `EGameThreadMethod.ProcessEvent` - 使用 ProcessEvent 钩子
- `EGameThreadMethod.EngineTick` - 使用 EngineTick 钩子（每帧一次）

**ExecuteInGameThread 增强：**
`ExecuteInGameThread` 现在接受可选的第二个参数来指定执行方式：

```lua
-- 默认行为（使用配置项）
ExecuteInGameThread(function() print("Hello\n") end)

-- 显式选择方式
ExecuteInGameThread(function() print("Hello\n") end, EGameThreadMethod.EngineTick)
ExecuteInGameThread(function() print("Hello\n") end, EGameThreadMethod.ProcessEvent)
```

**用法示例：**

```lua
-- 简单延迟
local handle = ExecuteInGameThreadWithDelay(1000, function()
    print("Fired after 1 second\n")
end)

-- 可重触发计时器（每次调用都会重置）
local debounceHandle = MakeActionHandle()
RegisterKeyBind(Key.F, function()
    RetriggerableExecuteInGameThreadWithDelay(debounceHandle, 500, function()
        print("Debounced action\n")
    end)
end)

-- 自我取消的循环
local loopHandle
loopHandle = LoopInGameThreadWithDelay(1000, function()
    print("Tick!\n")
    if someCondition then
        CancelDelayedAction(loopHandle)
    end
end)
```

**重要说明：**

- 基于句柄的归属机制可防止不同 mod 的计时器互相干扰
- `ClearAllDelayedActions()` 只清理调用它的 mod 的操作
- 基于帧的延迟需要 EngineTick 钩子（请检查 `EngineTickAvailable`）
- 在闭包中捕获循环句柄时，要先声明变量再赋值：

  ```lua
  local loopHandle  -- 先声明
  loopHandle = LoopInGameThreadWithDelay(1000, function()
      CancelDelayedAction(loopHandle)  -- 现在可以被正确捕获
  end)
  ```

## 反馈迁移问题

如果你在升级过程中遇到问题，请：

1. 在[已有 issue](https://github.com/UE4SS-RE/RE-UE4SS/issues) 中查找类似报告
2. 如果你的问题尚未被报告，请新建一个升级问题 issue
