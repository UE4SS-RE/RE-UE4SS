# Dumpers

## C++ Header Generator

The C++ dumper is a tool that generates C++ headers from UE4 classes and blueprints.

The keybind to generate headers is by default `CTRL` + `H`, and it can be changed in `Mods/Keybinds/Scripts/main.lua`.

It generates a `.hpp` file for each blueprint (including animation blueprint and widget blueprint), and then all of the base classes inside of `<ProjectName>.hpp` or `<EngineModule>.hpp`. All classes are at the top of the files, followed by all structs. Enums are seperated into files named the same as their class, but with `_enums` appended to the end.

### Configurations
- `DumpOffsetsAndSizes` (bool)
    - Whether to property offsets and sizes
    - Default: 1

- `KeepMemoryLayout` (bool)
    - Whether memory layouts of classes and structs should be accurate
    - This must be set to 1, if you want to use the generated headers in an actual C++ project
    - When set to 0, padding member variables will not be generated
    - Default: 0
    > Warning: A value of 1 has no purpose yet as memory value is not accurate either way!

- `LoadAllAssetsBeforeGeneratingCXXHeaders` (bool)
    - Whether to force all assets to be loaded before generating headers
    - Default: 0
    > Warning: Can require multiple gigabytes of extra memory, is not stable & will crash the game if you load past the main menu after dumping

## Unreal Header Tool (UHT) Dumper

Generates Unreal Header Tool compatible C++ headers for creating a mirror `.uproject` for your game. The guide for using these headers can be found [here](../guides/generating-uht-compatible-headers.md).

The keybind to generate headers is by default `CTRL` + `Numpad 9`, and it can be changed in `Mods/Keybinds/Scripts/main.lua`.

### Configurations
- `IgnoreAllCoreEngineModules` (bool)
    - Whether to skip generating packages that belong to the engine, particularly useful for any games that make alterations to the engine
    - Default: 0

- `IgnoreEngineAndCoreUObject` (bool)
    - Whether to skip generating the `Engine` and `CoreUObject` packages
    - Default: 1

- `MakeAllFunctionsBlueprintCallable` (bool)
    - Whether to force all UFUNCTION macros to have `BlueprintCallable`
    - Default: 1
    > Warning: This will cause some errors in the generated headers that you will need to manually fix

- `MakeAllPropertyBlueprintsReadWrite` (bool)
    - Whether to force all UPROPERTY macros to have `BlueprintReadWrite`
    - Also forces all UPROPERTY macros to have `meta=(AllowPrivateAccess=true)`
    - Default: 1

- `MakeEnumClassesBlueprintType` (bool)
    - Whether to force UENUM macros on enums to have `BlueprintType` if the underlying type was implicit or uint8
    - Default: 1
    > Warning: This also forces the underlying type to be uint8 where the type would otherwise be implicit

- `MakeAllConfigsEngineConfig` (bool)
    - Whether to force `Config = Engine` on all UCLASS macros that use either one of: `DefaultConfig`, `GlobalUserConfig` or `ProjectUserConfig`
    - Default: 1

## Object Dumper

Dumps all loaded objects to the file `UE4SS_ObjectDump.txt` (you can turn on force loading for all assets). 

The keybind to dump objects is by default `CTRL` + `J`, and can be changed in `Mods/Keybinds/Scripts/main.lua`.

Example output:
```
[000002A70F57E5C0] Function /Game/UI/Art/WidgetParts/Basic_ButtonScalable2.Basic_ButtonScalable2_C:BndEvt__Button_0_K2Node_ComponentBoundEvent_0_OnButtonClickedEvent__DelegateSignature [n: 5343AA] [c: 000002A727993A00] [or: 000002A708466980]
[000002A70F57E4E0] Function /Game/UI/Art/WidgetParts/Basic_ButtonScalable2.Basic_ButtonScalable2_C:PreConstruct [n: 4057B] [c: 000002A727993A00] [or: 000002A708466980]
[000002A70F876600] BoolProperty /Game/UI/Art/WidgetParts/Basic_ButtonScalable2.Basic_ButtonScalable2_C:PreConstruct:IsDesignTime [o: 0] [n: 4D63DB] [c: 00007FF683722CC0] [owr: 000002A70F57E4E0]
```
There are multiple sets of opening & closing square brackets and each set has a different meaning and the letters in this table explains what they mean.  
Within the first set of brackets is the location in memory where the object or property is stored.

| Letters | Meaning                                                                               | UE Member Variable |
|---------|---------------------------------------------------------------------------------------|--------------------|
| n       | Name of an object/property                                                            | NamePrivate        |
| c       | Class of the object/property/enum value                                               | ClassPrivate       |
| or      | Outer of the object                                                                   | OuterPrivate       | 
| o       | Offset of a property value in an object                                               | Offset_Internal    |
| owr     | Owner of an FField, 4.25+ only                                                        | Owner              |
| kp      | Key property of an FMapProperty                                                       | KeyProp            |
| vp      | Value property of an FMapProperty                                                     | ValueProp          |
| mc      | Class that this FClassProperty refers to                                              | MetaClass          |
| df      | Function that this FDelegateProperty refers to                                        | FunctionSignature  |
| pc      | Class that this FObjectProperty/FFieldPathProperty refers to                          | PropertyClass      |
| ic      | Class that this FInterfaceProperty refers to                                          | InterfaceClass     |
| ss      | Struct that this FStructProperty refers to                                            | Struct             |
| em      | Enum that this FEnumProperty refers to                                                | Enum               |
| fm      | Field mask that this FBoolProperty refers to                                          | FieldMask          |
| bm      | Byte mask that this FBoolProperty refers to                                           | ByteMask           |
| v       | Value corresponding to this enum key                                                  | N/A                |
| sps     | SuperStruct of this UClass                                                            | SuperStruct        |
| ai      | Property that this FArrayProperty stores                                              | Inner              |
| f       | Refers to the native function wrapper of a UFunction or ProcessInternal if non-native | Func               |

### Configurations

- `LoadAllAssetsBeforeDumpingObjects` (bool)
    - Whether to force all assets to be loaded before dumping objects
    - Default: 0
    > Warning: Can require multiple gigabytes of extra memory, is not stable & will crash the game if you load past the main menu after dumping

## .usmap Dumper

Generate `.usmap` mapping files for unversioned properties. 

The keybind to dump mappings is by default `Ctrl` + `Numpad 6`, and can be changed in `Mods/Keybinds/Scripts/main.lua`.

Thanks to [OutTheShade](https://github.com/OutTheShade/UnrealMappingsDumper) for the original implementation.

## .umap Recreation Dumper

Dump all loaded actors to the file `ue4ss_static_mesh_data.csv` to generate `.umaps` in-editor. 

Two prerequisites are required to load the dumped actors in-editor to reconstruct the `.umap`:
- All dumped actors (static meshes, their materials/textures) must be reconstructed in the editor
- Download `zMapGenBP.zip` from the Releases page and follow the instructions in the Readme file inside of it

The keybind to dump mappings is by default `Ctrl` + `Numpad 7`, and can be changed in `Mods/Keybinds/Scripts/main.lua`.
