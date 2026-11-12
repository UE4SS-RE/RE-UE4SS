# 在 C++ 中操作 UE 属性

制作 mod 时，你很可能需要读写 UE 属性。
传统且不推荐的做法是，要么自己定义与游戏内存布局一致的结构体，要么手动在基址指针上加上偏移。
这两种做法的缺点都是容易因为游戏或引擎更新而失效。
更好的做法是使用我们为 C++ mod 提供的 API。
这些 API 不依赖编译期已知的结构体，也不依赖容易失效的偏移量。
它们利用的是每个 UE 游戏都提供的元数据。

## 模板化 API

我们提供了一些模板化函数，让你更容易获取想要的类型。
这些是访问属性值最简单的方式。

### UObject::GetValuePtrByPropertyNameInChain

该函数返回指向属性值的指针。
它会在整个类继承层级中搜索 'PropertyName'，因此推荐优先使用它，而不是不带 InChain 的版本。

```c++
// 本例中，假设这个 UObject* 已通过某种方式正确获取，且不为 nullptr。
UObject* PlayerController{};
FVector* SpawnLocation = PlayerController->GetValuePtrByPropertyNameInChain<FVector>(STR("SpawnLocation"));
Output::send(STR("{} {} {}\n"), SpawnLocation->X(), SpawnLocation->Y(), SpawnLocation->Z());
```

### UObject::GetValuePtrByPropertyName

与 [UObject::GetValuePtrByPropertyNameInChain](#uobjectgetvalueptrbypropertynameinchain) 相同，只是它只检查最内层的类。
例如对 `APlayerController` 而言，它只会在 `APlayerController` 内部查找，而不会查找 `AController` 或其他它继承的类。

## 访问 UStruct 的值

不推荐自己创建模仿游戏结构体的 C++ 结构体。

相反，你应该利用 UE 的元数据来逐个成员获取指针。
这样可以避免代码随游戏更新而失效，同时也更简单，因为不需要逆向工程。
我们提供了一些不会失效的结构体（例如 `FVector` 与 `FRotator`），但在大多数情况下我们并不提供。

下面是一些示例代码，展示如何从 UObject 内部的结构体中获取值。

```c++
// 获取：Engine->TinyFontName.SubPathString
// 本例中，假设这个 UObject* 已通过某种方式正确获取，且不为 nullptr。
UObject* Engine{};
// 1. 获取 TinyFontName 变量的属性。
//    就游戏而言它的真实类型是 FSoftObjectPath。
//    UE4SS 确实提供了 'FSoftObjectPath' 结构体，但我们假装没有提供。
auto TinyFontNameProperty = static_cast<FStructProperty*>(Engine->GetPropertyByNameInChain(STR("TinyFontName")));
// 2. 获取与该属性对应的 UStruct。
UStruct* SoftObjectPath = TinyFontNameProperty->GetStruct();
// 3. 获取指向 UEngine 实例内部 TinyFontName 值的指针。
auto TinyFontName = TinyFontNameProperty->ContainerPtrToValuePtr<void>(Engine);
// 4. 获取 FSoftObjectPath UStruct 中 SubPathString 的属性。
FProperty* SubPathStringProperty = SoftObjectPath->GetPropertyByNameInChain(STR("SubPathString"));
// 5. 获取 FSoftObjectPath 结构体实例 'TinyFontName' 内部 SubPathString 值的指针。
//    注意这里我们提供了模板参数，因为已知它是 FString，而 UE4SS 有安全的 FString 实现。
//    若不提供模板参数，则返回 void*。
auto SubPathString = SubPathStringProperty->ContainerPtrToValuePtr<FString>(TinyFontName);
// 6. 输出该值。
Output::send(STR("TinyFontName: {}\n"), SubPathString->GetCharArray());
```

你可以这样任意深入多少层容器，也就是说结构体套结构体也没有问题。
例如：

```c++
// 获取：Engine->TinyFontName.AssetPath.AssetName
// 承接上面的示例。
// 7. 获取 FSoftObjectPath UStruct 中 AssetPath 变量的属性。
//    就游戏而言它的真实类型是 FTopLevelAssetPath。
auto AssetPathProperty = static_cast<FStructProperty*>(SoftObjectPath->GetPropertyByNameInChain(STR("AssetPath")));
// 8. 获取与该属性对应的 UStruct。
auto TopLevelAssetPath = AssetPathProperty->GetStruct();
// 9. 获取 FSoftObjectPath 结构体实例 'TinyFontName' 内部 AssetPath 值的指针。
auto AssetPath = AssetPathProperty->ContainerPtrToValuePtr<void>(TinyFontName);
// 10. 获取 FTopLevelAssetPath UStruct 中 AssetName 变量的属性。
auto AssetNameProperty = TopLevelAssetPath->GetPropertyByNameInChain(STR("AssetName"));
// 11. 获取 FTopLevelAssetPath 结构体实例 'AssetPath' 内部 AssetName 值的指针。
auto AssetName = AssetNameProperty->ContainerPtrToValuePtr<FName>(AssetPath);
// 12. 输出该值。
Output::send(STR("AssetName: {}\n"), AssetName->ToString());
```

`ContainerPtrToValuePtr` 函数总是返回指向 `T` 的指针。
在上面的示例中，`T` == `FName`，因此该函数返回 `FName*`。
由于它总是返回指针，修改属性值非常容易：

```c++
// 承接上面的示例。
// 13. 通过解引用指针并把值设为新的 FName，把 'AssetName' 的值设为 "THIS IS A TEST"。
*AssetName = FName(STR("THIS IS A TEST"), FNAME_Add);
// 14. 输出新值。
Output::send(STR("AssetName: {}\n"), AssetName->ToString());
```
