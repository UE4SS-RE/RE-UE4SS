# Blueprint Macros

The following macros are used to manipulate blueprint functions from C++.

> **Note:** Param names for wrappers must be identical to the names used for the function in UE, and they should then be passed to macros with a `PropertyName` param as shown in `AActor.cpp`.

This does not apply to macros with the `_CUSTOM` suffix.

With those macros you have to supply both the UE property name as well as the name of your C++ param.

These `_CUSTOM` suffixed macros are useful when the UE property name contains spaces or other characters that aren't valid for a C++ variable.

## Regular macros:
Intended for normal use by modders.

### `UE_BEGIN_SCRIPT_FUNCTION_BODY`: 
Finds non-native (meaning BP) UFunction by its full name without the type prefixed, throws if not found.


### `UE_BEGIN_NATIVE_FUNCTION_BODY`: 
Same as above except for native, meaning non-BP UFunctions.

See: `AActor::K2_DestroyActor`


### `UE_SET_STATIC_SELF`: 
Used for static functions, and should be the CDO to the class that the UFunction belongs to.

See: `UKismetNodeHelperLibrary::GetEnumeratorUserFriendlyName.`


### `UE_COPY_PROPERTY`: 
Copies the property of the supplied name into the already allocated params struct.
- Param 1: The name, without quotes, of a property that exists for this UFunction.
- Param 2: The type that you want the underlying value to be copied as. For example, without quotes, "float" for `FFloatProperty`.


### `UE_COPY_PROPERTY_CUSTOM`: 
Copies the property of the supplied name into the already allocated params struct.
- Param 1: The name, without quotes, of a property that exists for this UFunction.
- Param 2: A C++ compatible variable name for the property.
- Param 3: The type that you want the underlying value to be copied as. For example, without quotes, "float" for `FFloatProperty`.


### `UE_COPY_STRUCT_PROPERTY_BEGIN`: 
Begins the process of copying an entire struct.
- Param 1: The name, without quotes, of an ``FStructProperty`` that exists for this UFunction.


### `UE_COPY_STRUCT_PROPERTY_CUSTOM_BEGIN`: 
Begins the process of copying an entire struct.
- Param 1: The name, without quotes, of an ``FStructProperty`` that exists for this UFunction.
- Param 2: A C++ compatible variable name for the property.


### `UE_COPY_STRUCT_INNER_PROPERTY`: 
Copies a property from within an `FStructProperty` into the already allocated params struct.
- Param 1: The name, without quotes, of the `FStructProperty` supplied to `UE_COPY_STRUCT_PROPERTY_BEGIN`.
- Param 2: The name, without quotes, of a property that exists in the supplied `FStructProperty`.
- Param 3: The type that you want the underlying value to be copied as. For example, without quotes, "float" for `FFloatProperty`.
- Param 4: The name of the C++ variable that you're copying.

See: `AActor::K2_SetActorRotation`


### `UE_COPY_STRUCT_INNER_PROPERTY_CUSTOM`:
- Param 1: The name, without quotes, of the `FStructProperty` supplied to `UE_COPY_STRUCT_PROPERTY_BEGIN`.
- Param 2: The name, without quotes, of a property that exists in the supplied `FStructProperty`.
- Param 3: A C++ compatible variable name for the property.
- Param 4: The type that you want the underlying value to be copied as. For example, without quotes, "float" for `FFloatProperty`.
- Param 5: The name of the C++ variable that you're copying.


### `UE_COPY_OUT_PROPERTY`: 
Copies the out property of the supplied name from the params struct into the supplied C++ variable.

This means the wrapper param (which is named the same as the property supplied) must be a reference, meaning suffixed with a "&".
- Param 1: The name, without quotes, of a property that exists for this UFunction.
- Param 2: The type that you want the underlying value to be copied as. For example, without quotes, "float" for `FFloatProperty`.

See: `UGameplayStatics::FindNearestActor`


### `UE_COPY_OUT_PROPERTY_CUSTOM`: 
Copies the out property of the supplied name from the params struct into the supplied C++ variable.
- Param 1: The name, without quotes, of a property that exists for this UFunction.
- Param 2: A C++ compatible variable name for the property.
- Param 3: The type that you want the underlying value to be copied as. For example, without quotes, "float" for `FFloatProperty`.

This means the wrapper param (which is named the same as the property supplied) must be a reference, meaning suffixed with a "&".


### `UE_COPY_VECTOR:` 
Helper for copying an FVector. Must use `UE_COPY_STRUCT_PROPERTY_BEGIN` first.
- Param 1: The C++ name, without quotes, of the FVector to copy from.
- Param 2: The name, without quotes, of the FVector, same as supplied to `UE_COPY_STRUCT_PROPERTY_BEGIN`.


### `UE_COPY_STL_VECTOR_AS_TARRAY`: 
Helper for copying a TArray.
- Param 1: The name, without quotes, of an `FArrayProperty` that exists for this UFunction.
- Param 2: The C++ type, without quotes, that the TArray holds. For example, without quotes, "float", for `FFloatProperty`.
- Param 3: The C++ that the contents of the TArray will be copied into.


### `UE_CALL_FUNCTION`: 
Performs a non-static function call. All non-out params must be copied ahead of this.


### `UE_CALL_STATIC_FUNCTION`: 
Performs a static function call, using the CDO provided by `UE_SET_STATIC_SELF` as the static instance. All non-out params must be copied ahead of this.


### `UE_RETURN_PROPERTY`: 
Copies the underlying value that the UFunction returned and returns it.
- Param 1: The type that you want the underlying value to be copied as. For example, without quotes, "float" for `FFloatProperty`.


### `UE_RETURN_PROPERTY_CUSTOM`:
- Param 1: The type that you want the underlying value to be copied as. For example, without quotes, "float" for `FFloatProperty`.
- Param 2: The name, without quotes, for the property of this function where the return value will be copied from.


### `UE_RETURN_VECTOR`: 
Helper for returning an `FVector`.


### `UE_RETURN_STRING`: 
Helper for returning an `FStrProperty`. Converts to `StringType`.


### `UE_RETURN_STRING_CUSTOM`: 
Helper for returning an `FStrProperty`. Converts to `StringType`.
- Param 1: The name, without quotes, for the `FStrProperty` of this function where the return value will be copied from.


### `WITH_OUTER`: 
Used for templated C++ types passed to macros, like TArray or TMap.

For example, pass, without quotes, `WITH_OUTER(TMap, FName, int)` instead of `TMap<FName, int>` to all macros.

## Internal macros

These are only used by other macros, or by users of our C++ API if they properly understand the internals of the macros, and this requires preexisting knowledge around how UFunctions work, and you'll likely have to [BPMacros.hpp](https://github.com/Re-UE4SS/UEPseudo/blob/main/include/Unreal/BPMacros.hpp) to understand how to use them properly.

### `UE_BEGIN_FUNCTION_BODY_INTERNAL`: 
Throws if the UFunction doesn't exist, and allocates enough space (on the stack when possible, otherwise the heap) for the params and return value(s).


### `UE_COPY_PROPERTY_INTERNAL`: 
Finds the property, and throws if not found.


### `UE_COPY_PROPERTY_CUSTOM_INTERNAL`: 
Finds the property with the supplied name, and throws if not found.


### `UE_RETURN_PROPERTY_INTERNAL`: 
Finds the property to be used for the return value, throws if not found.
