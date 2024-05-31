# DataTables in UE4SS

## Background

[DataTables](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/UDataTable/) are a data structure in Unreal Engine that allows for hashed key-value pairs to be loaded at runtime. Common use cases include storing loot tables, experience point requirements for leveling up, base health/armor for actors, etc...

DataTables are intended to be populating as part of game compilation and aren't *technically* supposed to be modified at runtime. The documentation from Unreal sometimes contradicts this statement, so it's a bit hard to parse what's *intended* versus what's *possible*. My goal is to allow for full read/write/update/delete/iterate operations at runtime from a C++ context without the use of blueprints.

## Why not just create a blueprint mod that replaces a DataTable?

This *technically* works. The problem is that your mod is the **only** mod that can change this DataTable. This is obviously not ideal for clients that want to use multiple mods that want to modify the same DataTable. I rate this solution around a 2/10 from a extensibility perspective.

## What is the structure of a DataTable?

DataTables are build by using `TMap` and `TSet` from native Unreal. If you are familiar with Java's `HashMap` or C#'s `Dictionary` then you'll understand the gist of the contracts/usage. Unreal `DataTable` has keys of [`FName`](https://docs.unrealengine.com/4.26/en-US/ProgrammingAndScripting/ProgrammingWithCPP/UnrealArchitecture/StringHandling/FName/) and the value is a struct that inherits from [`FTableRowBase`](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Engine/FTableRowBase/). More on this later...

## So what needs to be done?

I will outline a couple of possibilities for the modification of DataTables. I will be evaluating the feasibility/stability of each proposed solution to give some perspective.

### Solution 1 (TMap implementation)

A DataTable in Unreal Engine exposes a RowMap property that can be accessed:
```cpp
// DataTable.h
virtual const TMap< FName, uint8 * > & GetRowMap() const 
virtual const TMap< FName, uint8 * > & GetRowMap()  
```
The GetRowMap() function is reflected and is easily callable by using the UVTD files. The problem is that UE4SS has a bare-bones implementation of TMap. The current TMap implementation in UE4SS can be leveraged in the following manner:
```cpp
// DataTable row format is <FName, CoolStruct>
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

**So what's the big deal?**

UE4SS's TMap does not like when the underlying data is changed. This way of accessing data works reasonably well for DataTable reads/iterators, but after we call `dt->AddRow()` or `dt->RemoveRow()`, the underlying `.GetElementsPtr()` is inaccurate. If you look at the UE4SS implementation of TMap, you can see that it's fairly fragile unless you intend to read only.

Note that the current `.Num()` function in UE4SS TMap does not actually perform calculations on the TMap. The `Num` property is just set when we construct a TMap in UE4SS, so we don't get updates when the underlying size changes.

I suppose this solution is reasonable for reading a DataTable if that's all you want to do.

So how can we make this work?

Theoretically we can implement TMap in UE4SS with mirrored functionality to UE native. UE4SS has done a similar approach with `TArray`. The potential downsides are that if TMap underlying logic/structures have changed between UE versions, then we would need multiple implementations that represent the state of UE TMaps at different versions. Either that, or, we could have `#if UE5_1` etc. to keep things consolidated in a single TMap.hpp/cpp file.

Will implementing TMap in UE4SS work for modifying DataTables? I haven't completed a thorough investigation, but my gut says... *probably?*

**Why can't we use FindRow/GetRow on the DataTable object?**

The only useful reflected functions we get from `UDataTable` dump is `GetRowMap()`, `RemoveRow()`, and `AddRow()`. Not too shabby, but unfortunate that we can't get a row directly or use a UE4SS `TMap` to get a row.


### Solution 2 (Kismet DataTable Helper Library)
This approach leverages a blueprint [DataTable helper class](https://docs.unrealengine.com/4.27/en-US/API/Runtime/Engine/Kismet/UDataTableFunctionLibrary/) built into Unreal Engine. The reflected functions from this blueprint helper are:
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

If you've been paying attention, then a light bulb might be going off in your head. Seems like we could accomplish full DataTable support by utilizing
```cpp
// DataTable reflected functions
AddRow();
RemoveRow();
Empty();

// DataTableFunctionLibrary reflected functions
DoesDataTableRowExist();
GetDataTableRowNames();
GetDataTableRowFromName();
```

**But there's always a catch...**

`GetDataTableRowFromName();` is an especially cursed function. The TLDR is that it's *probably* usable, but will require some further experimentation.

This next section benefits from somewhat of an intimate knowledge of how Kismet/blueprints/FFrame and the blueprint scripting stack works. I'll include some pre-reads to familiarize yourself.

* [Custom Thunks TLDR](https://gist.github.com/intaxwashere/e9b1f798427686b46beab2521d7efbcf)
* [Blueprints from C++](https://intaxwashere.github.io/blueprint-access/)
* [Blueprint Function Templates](https://neil3d.github.io/unreal/blueprint-wildcard.html)

`GetDataTableRowFromName()` has the [specifiers](https://docs.unrealengine.com/4.26/en-US/ProgrammingAndScripting/GameplayArchitecture/Functions/Specifiers/) `CustomThunk` and `CustomStructureParam`.

```
CustomThunk:
The UnrealHeaderTool code generator will not produce a thunk for this function; it is up to the user to provide one with the DECLARE_FUNCTION or DEFINE_FUNCTION macros.

CustomStructureParam:
The listed parameters are all treated as wildcards. This specifier requires the UFUNCTION-level specifier, CustomThunk, which will require the user to provide a custom exec function. In this function, the parameter types can be checked and the appropriate function calls can be made based on those parameter types. The base UFUNCTION should never be called, and should assert or log an error if it is. 
```

Under the hood, the `GetDataTableRowFromName()` UFunction is just a stub. The DataTableFunctionLibrary provides the actual behavior with a `DEFINE_FUNCTION(execGetDataTableRowFromName)` macro. Let's take a look at what the defined function is:
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
		// The following line fails to find the StructProp. See notes below this code block for the specifics.
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

The issue is that the Stack.MostRecentProperty does not get populated when we call the `GetDataTableRowFromName()` from a C++ context. This specifics of this have been documented at by the following GitHub issues:

* [Issue 1](https://github.com/Tencent/puerts/issues/836) (CN)
* [Issue 2](https://github.com/Tencent/puerts/issues/986) (CN)

Under the hood:
```cpp
static bool GetDataTableRowFromName
(
    UDataTable * Table,
    FName RowName,
    FTableRowBase & OutRow
)

// Does some property reading, type checking, etc,
// Then internally it calls

static bool Generic_GetDataTableRowFromName
(
    const UDataTable * Table,
    FName RowName,
    void * OutRowPtr
) 
```

It would be suitable for us to use a `void*` for the `OutRow` instead of a ref `FTableRowBase`, but as fate would have it, this `Generic_GetDataTableRowFromName()` is not accessible via reflection.

The core of the problem is that the `execGetDataTableRowFromName()` is particularly aggressive at typechecking and ensuring that the function will work or gracefully exit. This is expected since this function is a blueprint node and needs to be a robust function to work within the blueprint framework. The specific way that `Stack.MostRecentProperty` is used is to determine the target type of Struct that we expect to retrieve from the DataTable. In the blueprint caller context, this property would be populated as part of the Kismet FFrame/Stack pipeline.

**Anything we can do?**

I am currently playing with manually setting the `Stack.MostRecentProperty` to trick the `GetDataTableRowFromName()` into *thinking* that we're calling the function as part of a legal blueprint function and not directly from C++ code. Like solution 1, I rate this solution as a *probably?* in the functionality department.

### One final wrench in the machine...
There's also further research needed about how DataTable row structs are stored in memory. It appears some games might have compiler packing, but the extent of this is still unknown. Furthermore, some games have reasonably laid out struct members for memory footprint/alignment/padding purposes, and other games have their struct members in a way that makes sense from a readability standpoint, but not from a memory optimization standpoint. 

```cpp
// NameTypes.hpp (UE4SS)

// TODO:   Figure out what's going on here
//         It shouldn't be required to use 'alignas' here to make sure it's aligned properly in containers (like TArray)
//         I've never seen an FName not be 8-byte aligned in memory,
//         but it is 4-byte aligned in the source so hopefully this doesn't cause any problems
// UPDATE: This matters in the UE VM, when ElementSize is 0xC in memory for case-preserving games, it must be aligned by 0x4 in that case
#pragma warning(disable: 4324) // Suppressing warning about struct alignment
#ifdef WITH_CASE_PRESERVING_NAME
    struct alignas(4) RC_UE_API FName
#else
    struct alignas(8) RC_UE_API FName // FNames in DataTable rows seem to only work with alignas(4)
```

The above code is a TODO: that's still in UE4SS. The investigation of alignment will likely have benefits across other non-DataTable parts! We'll need to understand the full extent of alignment/padding regardless of which solution we use (TMap or Blueprint Library or Other).

## Disclaimer
While I feel that I have a good understanding of the factors at play, I have no doubt that I've missed some of the nuance and have misunderstood parts of the underlying systems. Please let me know if you think something operates differently than is currently documented. I would really appreciate the help!

## Got any ideas?
Please reach out in the UE4SS Discord to brainstorm/share any ideas you might have. While I am currently in the role as feature lead for DataTables, I appreciate all the help I can get.

## Other Resources

* [DataTable Pull Request](https://github.com/Re-UE4SS/UEPseudo/pull/74/commits/04ef456ea337c59bc2df8ba9162ce4a7e52de445) - I think you need Epic Games group access to view this?
* [UE5 Wiki](https://ue5wiki.com/wiki/34184/) (CN)
* [UE4SS Docs](https://docs.ue4ss.com/)
* [JIP Blog](https://jip.dev/notes/unreal-engine/)

## Credits
Special thanks to localcc for being a wonderful mentor. Shout out to all early adopters of the DataTable branches (special thanks to El for being our first early adopter).


Thanks for your continued patience. 

-- bitonality