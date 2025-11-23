# Interacting with UE properties in C++

When making a mod, there's a very good chance that you're going to need to read or write a UE property.  
The classic and not recommended methods to do this is to either set up your own structs that match the memory of the
game, or manually add an offset to the base pointer.  
Both these methods come with the downside of easily becoming out-of-date with the game from game or engine updates.  
The better method to interact with properties is to use the APIs that we provide to C++ mods.  
These APIs don't rely on a struct known at compile-time, or offsets that can easily become out of date.  
It does this by using the metadata available in every UE game.

## Templated APIs

We supply templated functions to make it easier to retrieve the type you want.  
These are the easiest ways to access property values.

### UObject::GetValuePtrByPropertyNameInChain

This function returns a pointer to the value for the property.  
It searches the entire class hierarchy (inheritance) for 'PropertyName', and because of this, it's recommended over the
non-InChain variant.

```c++
// For this exmaple, assume that this UObject* is correctly retrieved in some way, and isn't nullptr.
UObject* PlayerController{};
FVector* SpawnLocation = PlayerController->GetValuePtrByPropertyNameInChain<FVector>(STR("SpawnLocation"));
Output::send(STR("{} {} {}\n"), SpawnLocation->X(), SpawnLocation->Y(), SpawnLocation->Z());
```

### UObject::GetValuePtrByPropertyName

Same as [UObject::GetValuePtrByPropertyNameInChain](#uobjectgetvalueptrbypropertynameinchain), except it only checks the
innermost class.  
For example, for `APlayerController`, it will only look inside `APlayerController`, not `AController` or any other class
that it inherits from.

## Accessing UStruct values

It's not recommended to create your own C++ structs that mimic the game struct.  
Instead, you should make use of UEs metadata to retrieve pointers to each member individually.  
This is to prevent your code from becoming out-of-date with the game, and it's also easier because it doesn't require
reverse-engineering.  
We provide a few structs that don't become out-of-date, such as `FVector` and `FRotator`, but in most cases we don't.

Here's some example code showing how to retrieve a value from a struct inside a UObject.

```c++
// Retrieving: Engine->TinyFontName.SubPathString
// For this example, assume that this UObject* is correctly retrieved in some way, and isn't nullptr.
UObject* Engine{};
// 1. Retrieve the property for the TinyFontName variable.
//    The real type as far as the game is concerned is FSoftObjectPath.
//    We do provide an 'FSoftObjectPath' struct in UE4SS, but let's pretend that we don't.
auto TinyFontNameProperty = static_cast<FStructProperty*>(Engine->GetPropertyByNameInChain(STR("TinyFontName")));
// 2. Retrieve the UStruct that corresponds with the property.
UStruct* SoftObjectPath = TinyFontNameProperty->GetStruct();
// 3. Retrieve a pointer to the TinyFontName value inside the UEngine instance.
auto TinyFontName = TinyFontNameProperty->ContainerPtrToValuePtr<void>(Engine);
// 4. Retrieve the property for the SubPathString in the FSoftObjectPath UStruct.
FProperty* SubPathStringProperty = SoftObjectPath->GetPropertyByNameInChain(STR("SubPathString"));
// 5. Retrieve a pointer to the SubPathString value inside the FSoftObjectPath struct instance 'TinyFontName'.
//    Note that we're supplying a template parameter here because we know that it's an FString, and UE4SS has a safe FString implementation.
//    If you don't supply a template parameter, it will return a void* instead.
auto SubPathString = SubPathStringProperty->ContainerPtrToValuePtr<FString>(TinyFontName);
// 6. Output the value.
Output::send(STR("TinyFontName: {}\n"), SubPathString->GetCharArray());
```

You can take this any number of containers deep, meaning that structs in structs is not a problem.  
For example:

```c++
// Retrieving: Engine->TinyFontName.AssetPath.AssetName
// Continuing from the above example.
// 7. Retrieve the property for the AssetPath variable in the FSoftObjectPath UStruct.
//    The real type as far as the game is concerned is FTopLevelAssetPath.
auto AssetPathProperty = static_cast<FStructProperty*>(SoftObjectPath->GetPropertyByNameInChain(STR("AssetPath")));
// 8. Retrieve the UStruct that corresponds with the property.
auto TopLevelAssetPath = AssetPathProperty->GetStruct();
// 9. Retrieve a pointer to the AssetPath value inside the FSoftObjectPath struct instance 'TinyFontName'.
auto AssetPath = AssetPathProperty->ContainerPtrToValuePtr<void>(TinyFontName);
// 10. Retrieve the property for the AssetName variable in the FTopLevelAssetPath UStruct.
auto AssetNameProperty = TopLevelAssetPath->GetPropertyByNameInChain(STR("AssetName"));
// 11. Retrieve a pointer to the AssetName value inside the FTopLevelAssetPath struct instance 'AssetPath'.
auto AssetName = AssetNameProperty->ContainerPtrToValuePtr<FName>(AssetPath);
// 12. Output the value.
Output::send(STR("AssetName: {}\n"), AssetName->ToString());
```

The `ContainerPtrToValuePtr` function always returns a pointer to `T`.  
In the example above, `T` == `FName`, so the function returns `FName*`.  
Because it always returns a pointer, changing property values is trivial:

```c++
// Continuing from the above example.
// 13. Set the value of 'AssetName' to "THIS IS A TEST" by dereferencing the pointer and setting the value to a new FName.
*AssetName = FName(STR("THIS IS A TEST"), FNAME_Add);
// 14. Output the new value.
Output::send(STR("AssetName: {}\n"), AssetName->ToString());
```
