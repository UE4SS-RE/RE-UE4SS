# 蓝图宏

以下宏用于在 C++ 中操作蓝图函数。

> **注意：** 包装函数的参数名必须与 UE 中该函数使用的名称完全一致，随后应按 `AActor.cpp` 中的示例，把它们传给带有 `PropertyName` 参数的宏。

这不适用于带 `_CUSTOM` 后缀的宏。

使用这些宏时，你既需要提供 UE 属性名，也需要提供你的 C++ 参数名。

当 UE 属性名中包含空格或其他对 C++ 变量不合法的字符时，带 `_CUSTOM` 后缀的宏会很有用。

## 常规宏

供 mod 作者日常使用。

### `UE_BEGIN_SCRIPT_FUNCTION_BODY`

根据完整名称（不含类型前缀）查找非原生（即蓝图）UFunction，找不到则抛出异常。

### `UE_BEGIN_NATIVE_FUNCTION_BODY`

与上面相同，但用于原生（即非蓝图）UFunction。

参见：`AActor::K2_DestroyActor`

### `UE_SET_STATIC_SELF`

用于静态函数，应传入该 UFunction 所属类的 CDO。

参见：`UKismetNodeHelperLibrary::GetEnumeratorUserFriendlyName.`

### `UE_COPY_PROPERTY`

将指定名称的属性复制到已分配好的参数结构体中。

- 参数 1：该 UFunction 上存在的属性名，不加引号。
- 参数 2：你希望底层值被复制成的类型。例如，对于 `FFloatProperty` 填 `float`，不加引号。

### `UE_COPY_PROPERTY_CUSTOM`

将指定名称的属性复制到已分配好的参数结构体中。

- 参数 1：该 UFunction 上存在的属性名，不加引号。
- 参数 2：该属性对应的、符合 C++ 命名规则的变量名。
- 参数 3：你希望底层值被复制成的类型。例如，对于 `FFloatProperty` 填 `float`，不加引号。

### `UE_COPY_STRUCT_PROPERTY_BEGIN`

开始复制整个结构体。

- 参数 1：该 UFunction 上存在的 `FStructProperty` 名称，不加引号。

### `UE_COPY_STRUCT_PROPERTY_CUSTOM_BEGIN`

开始复制整个结构体。

- 参数 1：该 UFunction 上存在的 `FStructProperty` 名称，不加引号。
- 参数 2：该属性对应的、符合 C++ 命名规则的变量名。

### `UE_COPY_STRUCT_INNER_PROPERTY`

将 `FStructProperty` 内部的某个属性复制到已分配好的参数结构体中。

- 参数 1：传给 `UE_COPY_STRUCT_PROPERTY_BEGIN` 的 `FStructProperty` 名称，不加引号。
- 参数 2：该 `FStructProperty` 中存在的属性名，不加引号。
- 参数 3：你希望底层值被复制成的类型。例如，对于 `FFloatProperty` 填 `float`，不加引号。
- 参数 4：你正在复制的 C++ 变量名。

参见：`AActor::K2_SetActorRotation`

### `UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM`

- 参数 1：传给 `UE_COPY_STRUCT_PROPERTY_BEGIN` 的 `FStructProperty` 名称，不加引号。
- 参数 2：该 `FStructProperty` 中存在的属性名，不加引号。
- 参数 3：该属性对应的、符合 C++ 命名规则的变量名。
- 参数 4：你希望底层值被复制成的类型。例如，对于 `FFloatProperty` 填 `float`，不加引号。
- 参数 5：你正在复制的 C++ 变量名。

### `UE_COPY_OUT_PROPERTY`

将参数结构体中指定名称的 out 属性复制到提供的 C++ 变量中。

这意味着包装函数的参数（其名称与所传属性相同）必须是引用，即带有 `&` 后缀。

- 参数 1：该 UFunction 上存在的属性名，不加引号。
- 参数 2：你希望底层值被复制成的类型。例如，对于 `FFloatProperty` 填 `float`，不加引号。

参见：`UGameplayStatics::FindNearestActor`

### `UE_COPY_OUT_PROPERTY_CUSTOM`

将参数结构体中指定名称的 out 属性复制到提供的 C++ 变量中。

- 参数 1：该 UFunction 上存在的属性名，不加引号。
- 参数 2：该属性对应的、符合 C++ 命名规则的变量名。
- 参数 3：你希望底层值被复制成的类型。例如，对于 `FFloatProperty` 填 `float`，不加引号。

这意味着包装函数的参数（其名称与所传属性相同）必须是引用，即带有 `&` 后缀。

### `UE_COPY_VECTOR`

复制 FVector 的辅助宏。必须先使用 `UE_COPY_STRUCT_PROPERTY_BEGIN`。

- 参数 1：要复制来源的 FVector 的 C++ 名称，不加引号。
- 参数 2：该 FVector 的名称，与传给 `UE_COPY_STRUCT_PROPERTY_BEGIN` 的名称相同，不加引号。

### `UE_COPY_STL_VECTOR_AS_TARRAY`

复制 TArray 的辅助宏。

- 参数 1：该 UFunction 上存在的 `FArrayProperty` 名称，不加引号。
- 参数 2：该 TArray 所持有的 C++ 类型，不加引号。例如，对于 `FFloatProperty` 填 `float`，不加引号。
- 参数 3：TArray 的内容将被复制到的 C++ 对象。

### `UE_CALL_FUNCTION`

执行非静态函数调用。所有非 out 参数必须在此之前完成复制。

### `UE_CALL_STATIC_FUNCTION`

执行静态函数调用，使用由 `UE_SET_STATIC_SELF` 提供的 CDO 作为静态实例。所有非 out 参数必须在此之前完成复制。

### `UE_RETURN_PROPERTY`

复制 UFunction 返回的底层值并将其返回。

- 参数 1：你希望底层值被复制成的类型。例如，对于 `FFloatProperty` 填 `float`，不加引号。

### `UE_RETURN_PROPERTY_CUSTOM`

- 参数 1：你希望底层值被复制成的类型。例如，对于 `FFloatProperty` 填 `float`，不加引号。
- 参数 2：该函数中用于取返回值的属性名，不加引号。

### `UE_RETURN_VECTOR`

返回 `FVector` 的辅助宏。

### `UE_RETURN_STRING`

返回 `FStrProperty` 的辅助宏。会转换为 `StringType`。

### `UE_RETURN_STRING_CUSTOM`

返回 `FStrProperty` 的辅助宏。会转换为 `StringType`。

- 参数 1：该函数中用于取返回值的 `FStrProperty` 名称，不加引号。

### `WITH_OUTER`

用于传给宏的模板化 C++ 类型，例如 TArray 或 TMap。

例如，向所有宏传入 `WITH_OUTER(TMap, FName, int)`（不加引号），而不是 `TMap<FName, int>`。

## 内部宏

这些宏只被其他宏使用，或者被那些真正理解宏内部实现的 C++ API 使用者使用。这需要你事先了解 UFunction 的工作方式，并且很可能需要阅读 [BPMacros.hpp](https://github.com/Re-UE4SS/UEPseudo/blob/main/include/Unreal/BPMacros.hpp) 才能正确使用它们。

### `UE_BEGIN_FUNCTION_BODY_INTERNAL`

若 UFunction 不存在则抛出异常，并为参数与返回值分配足够的空间（可行时分配在栈上，否则分配在堆上）。

### `UE_COPY_PROPERTY_INTERNAL`

查找属性，找不到则抛出异常。

### `UE_COPY_PROPERTY_CUSTOM_INTERNAL`

按提供的名称查找属性，找不到则抛出异常。

### `UE_RETURN_PROPERTY_INTERNAL`

查找用于返回值的属性，找不到则抛出异常。
