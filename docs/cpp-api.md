# C++ API

These are the C++ API functions available in UE4SS, on top of the standard libraries that C++ comes with by default and the reflected functions available in Unreal Engine.

You are expected to have a basic understanding of C++ and Unreal Engine's C++ API before using these functions.

You may need to read code in the [UEPseudo](https://github.com/Re-UE4SS/UEPseudo) repository (more specifically, the `include/Unreal` directory) to understand how to use these functions.

For version: **4.0.0**

Current status: **in progress**

---

## Getting Started

### Creating a C++ Mod

C++ mods are DLLs that export two functions: `start_mod()` and `uninstall_mod()`. Your mod must inherit from `RC::CppUserModBase`.

```cpp
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>

class MyMod : public RC::CppUserModBase
{
public:
    MyMod() : CppUserModBase()
    {
        ModName = STR("MyMod");
        ModVersion = STR("1.0");
        ModDescription = STR("My custom mod");
        ModAuthors = STR("Your Name");
    }

    ~MyMod() override = default;

    auto on_unreal_init() -> void override
    {
        // Called when Unreal Engine is initialized
    }
};

#define MY_MOD_API __declspec(dllexport)
extern "C"
{
    MY_MOD_API RC::CppUserModBase* start_mod()
    {
        return new MyMod();
    }

    MY_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
```

### Required Exports

| Export | Signature | Description |
|--------|-----------|-------------|
| `start_mod` | `RC::CppUserModBase* start_mod()` | Called when the mod is loaded. Must return a new instance of your mod class. |
| `uninstall_mod` | `void uninstall_mod(RC::CppUserModBase* mod)` | Called when the mod is unloaded. Must delete the mod instance. |

---

## CppUserModBase Class

`RC::CppUserModBase` is the base class for all C++ mods.

### Mod Metadata Properties

| Property | Type | Description |
|----------|------|-------------|
| `ModName` | `StringType` | The name of your mod |
| `ModVersion` | `StringType` | Version string |
| `ModDescription` | `StringType` | Description of what your mod does |
| `ModAuthors` | `StringType` | Author(s) of the mod |
| `ModIntendedSDKVersion` | `StringType` | UE4SS SDK version this mod was built for (auto-populated if not set) |

### Lifecycle Callbacks

Override these virtual methods to hook into mod lifecycle events:

#### `on_update()`
```cpp
virtual auto on_update() -> void;
```
Called every frame. Use sparingly as this is performance-sensitive.

#### `on_unreal_init()`
```cpp
virtual auto on_unreal_init() -> void;
```
Called when the Unreal module has been initialized. Before this fires, you cannot **safely** use anything in the `Unreal` namespace.

#### `on_ui_init()`
```cpp
virtual auto on_ui_init() -> void;
```
Called when the UI module has been initialized. This is where you should use the `UE4SS_ENABLE_IMGUI` macro if you want to utilize the ImGui context of UE4SS.

#### `on_program_start()`
```cpp
virtual auto on_program_start() -> void;
```
Called when the UE4SS program has fully started.

#### `on_dll_load(StringViewType dll_name)`
```cpp
virtual auto on_dll_load(StringViewType dll_name) -> void;
```
Called when any DLL is loaded by the game. Useful for detecting when specific modules become available.

### Lua Integration Callbacks

These callbacks allow C++ mods to interact with Lua mods:

#### `on_lua_start()` (Named Mod)
```cpp
virtual auto on_lua_start(StringViewType mod_name,
                          LuaMadeSimple::Lua& lua,
                          LuaMadeSimple::Lua& main_lua,
                          LuaMadeSimple::Lua& async_lua,
                          LuaMadeSimple::Lua* hook_lua) -> void;
```
Executes after a Lua mod is started. Called for every Lua mod.

| Parameter | Description |
|-----------|-------------|
| `mod_name` | Name of the Lua mod that was started |
| `lua` | Main Lua instance |
| `main_lua` | Main Lua thread instance |
| `async_lua` | Lua instance for async operations (ExecuteAsync, ExecuteWithDelay) |
| `hook_lua` | Lua instance for game-thread hooks (ExecuteInGameThread) |

#### `on_lua_start()` (Same-Name Mod)
```cpp
virtual auto on_lua_start(LuaMadeSimple::Lua& lua,
                          LuaMadeSimple::Lua& main_lua,
                          LuaMadeSimple::Lua& async_lua,
                          LuaMadeSimple::Lua* hook_lua) -> void;
```
Executes after a Lua mod of the same name as your C++ mod is started.

#### `on_lua_stop()` (Named Mod)
```cpp
virtual auto on_lua_stop(StringViewType mod_name,
                         LuaMadeSimple::Lua& lua,
                         LuaMadeSimple::Lua& main_lua,
                         LuaMadeSimple::Lua& async_lua,
                         LuaMadeSimple::Lua* hook_lua) -> void;
```
Executes before a Lua mod is about to be stopped. Called for every Lua mod.

#### `on_lua_stop()` (Same-Name Mod)
```cpp
virtual auto on_lua_stop(LuaMadeSimple::Lua& lua,
                         LuaMadeSimple::Lua& main_lua,
                         LuaMadeSimple::Lua& async_lua,
                         LuaMadeSimple::Lua* hook_lua) -> void;
```
Executes before a Lua mod of the same name is about to be stopped.

### GUI Methods

#### `register_tab()`
```cpp
auto register_tab(StringViewType tab_name, GUI::GUITab::RenderFunctionType render_function) -> void;
```
Registers a new tab in the UE4SS debugging GUI.

**Parameters:**
- `tab_name`: The name displayed on the tab
- `render_function`: A function pointer with signature `void (*)(CppUserModBase*)`

**Example:**
```cpp
MyMod() : CppUserModBase()
{
    ModName = STR("MyMod");

    register_tab(STR("My Tab"), [](CppUserModBase* mod) {
        UE4SS_ENABLE_IMGUI()
        ImGui::Text("Hello from my mod!");
    });
}
```

### Input Methods

#### `register_keydown_event()` (Simple)
```cpp
auto register_keydown_event(Input::Key key,
                            const Input::EventCallbackCallable& callback,
                            uint8_t custom_data = 0) -> void;
```
Registers a callback for when a key is pressed.

**Example:**
```cpp
register_keydown_event(Input::Key::F5, []() {
    Output::send(STR("F5 was pressed!\n"));
});
```

#### `register_keydown_event()` (With Modifiers)
```cpp
auto register_keydown_event(Input::Key key,
                            const Input::Handler::ModifierKeyArray& modifier_keys,
                            const Input::EventCallbackCallable& callback,
                            uint8_t custom_data = 0) -> void;
```
Registers a callback for when a key is pressed with modifier keys.

**Example:**
```cpp
register_keydown_event(Input::Key::F5,
    {Input::ModifierKey::CONTROL, Input::ModifierKey::SHIFT},
    []() {
        Output::send(STR("Ctrl+Shift+F5 was pressed!\n"));
    });
```

---

## UE4SSProgram

The `UE4SSProgram` class provides access to the main UE4SS program instance and various utility functions.

### Accessing the Program Instance

```cpp
auto& program = RC::UE4SSProgram::get_program();
```

### Directory Methods

| Method | Return Type | Description |
|--------|-------------|-------------|
| `get_module_directory()` | `File::StringType` | Returns the directory containing the UE4SS module |
| `get_game_executable_directory()` | `File::StringType` | Returns the directory containing the game executable |
| `get_working_directory()` | `File::StringType` | Returns the current working directory |
| `get_mods_directory()` | `File::StringType` | Returns the primary mods directory path |
| `get_mods_directories()` | `std::vector<std::filesystem::path>&` | Returns all configured mod directories |
| `get_legacy_root_directory()` | `File::StringType` | Returns the legacy root directory (for backwards compatibility) |

### Mod Directory Management

```cpp
// Add a new mods directory
program.add_mods_directory(std::filesystem::path("path/to/mods"));

// Insert a mods directory at a specific index
program.insert_mods_directory(std::filesystem::path("path/to/mods"), 0);

// Remove a mods directory
program.remove_mods_directory(std::filesystem::path("path/to/mods"));
```

### Finding Mods

```cpp
// Find a Lua mod by name
auto* lua_mod = RC::UE4SSProgram::find_lua_mod_by_name(STR("ModName"));

// Find a Lua mod that is installed
auto* lua_mod = RC::UE4SSProgram::find_lua_mod_by_name(STR("ModName"),
    RC::UE4SSProgram::IsInstalled::Yes);

// Find a Lua mod that is started
auto* lua_mod = RC::UE4SSProgram::find_lua_mod_by_name(STR("ModName"),
    RC::UE4SSProgram::IsInstalled::Yes,
    RC::UE4SSProgram::IsStarted::Yes);

// Template versions for CppMod and LuaMod
auto* cpp_mod = RC::UE4SSProgram::find_mod_by_name<RC::CppMod>(STR("ModName"));
auto* lua_mod = RC::UE4SSProgram::find_mod_by_name<RC::LuaMod>(STR("ModName"));
```

### GUI Tab Management

```cpp
// Add a GUI tab
auto tab = std::make_shared<RC::GUI::GUITab>(STR("Tab Name"), render_function, this);
program.add_gui_tab(tab);

// Remove a GUI tab
program.remove_gui_tab(tab);
```

### Input Event Registration (Program Level)

```cpp
// Register a key event
program.register_keydown_event(Input::Key::F1, []() {
    // Handle key press
});

// Register with modifiers
program.register_keydown_event(Input::Key::F1,
    {Input::ModifierKey::CONTROL},
    []() {
        // Handle Ctrl+F1
    });

// Check if an event is registered
bool registered = program.is_keydown_event_registered(Input::Key::F1);
```

### Event Queue

```cpp
// Queue an event to be executed on the main thread
program.queue_event([](void* data) {
    // Execute on main thread
}, nullptr);

// Check if the queue is empty
bool empty = program.is_queue_empty();

// Check if events can be processed
bool can_process = program.can_process_events();
```

### Code Generation

```cpp
// Generate UHT-compatible headers
program.generate_uht_compatible_headers();

// Generate C++ headers to a specific directory
program.generate_cxx_headers(std::filesystem::path("output/dir"));

// Generate Lua type definitions
program.generate_lua_types(std::filesystem::path("output/dir"));
```

### Object Dumping

```cpp
// Dump all objects and properties to a file
RC::UE4SSProgram::dump_all_objects_and_properties(STR("path/to/output.txt"));
```

### ImGui Integration

When using ImGui in your mod, you must set up the context properly:

```cpp
auto on_ui_init() -> void override
{
    UE4SS_ENABLE_IMGUI()
    // Now you can use ImGui functions
}
```

Or manually:

```cpp
ImGuiContext* ctx = RC::UE4SSProgram::get_current_imgui_context();
ImGui::SetCurrentContext(ctx);

ImGuiMemAllocFunc alloc_func;
ImGuiMemFreeFunc free_func;
void* user_data;
RC::UE4SSProgram::get_current_imgui_allocator_functions(&alloc_func, &free_func, &user_data);
ImGui::SetAllocatorFunctions(alloc_func, free_func, user_data);
```

---

## Hook System

The hook system allows you to intercept various Unreal Engine functions. All hooks support both pre and post callbacks.

**Header:** `#include <Unreal/Hooks.hpp>`

### Hook Types

```cpp
namespace RC::Unreal::Hook
{
    enum class HookType
    {
        Pre,   // Called before the original function
        Post,  // Called after the original function
    };
}
```

### ProcessEvent Hooks

Intercept calls to `UObject::ProcessEvent`.

```cpp
using ProcessEventCallback = std::function<void(UObject* Context, UFunction* Function, void* Parms)>;

// Register callbacks
RC::Unreal::Hook::RegisterProcessEventPreCallback([](UObject* Context, UFunction* Function, void* Parms) {
    // Called before ProcessEvent
});

RC::Unreal::Hook::RegisterProcessEventPostCallback([](UObject* Context, UFunction* Function, void* Parms) {
    // Called after ProcessEvent
});
```

### StaticConstructObject Hooks

Intercept object construction.

```cpp
using StaticConstructObjectPreCallback = std::function<UObject*(const FStaticConstructObjectParameters& Params)>;
using StaticConstructObjectPostCallback = std::function<UObject*(const FStaticConstructObjectParameters& Params, UObject* ConstructedObject)>;

// Pre callback - can alter construction parameters
RC::Unreal::Hook::RegisterStaticConstructObjectPreCallback(
    [](const FStaticConstructObjectParameters& Params) -> UObject* {
        // Return nullptr to proceed normally, or return a different object
        return nullptr;
    });

// Post callback - object has been constructed
RC::Unreal::Hook::RegisterStaticConstructObjectPostCallback(
    [](const FStaticConstructObjectParameters& Params, UObject* ConstructedObject) -> UObject* {
        return ConstructedObject;
    });
```

### BeginPlay / EndPlay Hooks

Intercept actor lifecycle events.

```cpp
using BeginPlayCallback = std::function<void(AActor* Context)>;
using EndPlayCallback = std::function<void(AActor* Context, EEndPlayReason EndPlayReason)>;

RC::Unreal::Hook::RegisterBeginPlayPreCallback([](AActor* Context) {
    // Called before BeginPlay
});

RC::Unreal::Hook::RegisterBeginPlayPostCallback([](AActor* Context) {
    // Called after BeginPlay
});

RC::Unreal::Hook::RegisterEndPlayPreCallback([](AActor* Context, EEndPlayReason Reason) {
    // Called before EndPlay
});

RC::Unreal::Hook::RegisterEndPlayPostCallback([](AActor* Context, EEndPlayReason Reason) {
    // Called after EndPlay
});
```

### Tick Hooks

Intercept tick functions. **Warning:** These are extremely performance-sensitive.

```cpp
// Engine tick
using EngineTickCallback = std::function<void(UEngine* Context, float DeltaSeconds)>;
RC::Unreal::Hook::RegisterEngineTickPreCallback([](UEngine* Context, float DeltaSeconds) {});
RC::Unreal::Hook::RegisterEngineTickPostCallback([](UEngine* Context, float DeltaSeconds) {});

// Actor tick
using AActorTickCallback = std::function<void(AActor* Context, float DeltaSeconds)>;
RC::Unreal::Hook::RegisterAActorTickPreCallback([](AActor* Context, float DeltaSeconds) {});
RC::Unreal::Hook::RegisterAActorTickPostCallback([](AActor* Context, float DeltaSeconds) {});

// GameViewportClient tick
using GameViewportClientTickCallback = std::function<void(UGameViewportClient* Context, float DeltaSeconds)>;
RC::Unreal::Hook::RegisterGameViewportClientTickPreCallback([](UGameViewportClient* Context, float DeltaSeconds) {});
RC::Unreal::Hook::RegisterGameViewportClientTickPostCallback([](UGameViewportClient* Context, float DeltaSeconds) {});
```

### Console Exec Hooks

Intercept console command execution.

```cpp
// Simple callback (for UGameViewportClient::ProcessConsoleExec)
using ProcessConsoleExecCallback = std::function<bool(UObject* Context, const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor)>;

RC::Unreal::Hook::RegisterProcessConsoleExecCallback(
    [](UObject* Context, const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor) -> bool {
        // Return true if you handled the command, false otherwise
        return false;
    });

// Global callbacks with control over execution
using ProcessConsoleExecGlobalCallback = std::function<std::pair<bool, bool>(UObject* Context, const TCHAR* Cmd, FOutputDevice& Ar, UObject* Executor)>;
// Returns: {handled, skip_original_function}

RC::Unreal::Hook::RegisterProcessConsoleExecGlobalPreCallback(...);
RC::Unreal::Hook::RegisterProcessConsoleExecGlobalPostCallback(...);
```

### LoadMap Hooks

Intercept map loading.

```cpp
using LoadMapCallback = std::function<std::pair<bool, bool>(UEngine*, FWorldContext& WorldContext, FURL URL, UPendingNetGame* PendingGame, FString& Error)>;
// Returns: {handled, skip_original_function}

RC::Unreal::Hook::RegisterLoadMapPreCallback(...);
RC::Unreal::Hook::RegisterLoadMapPostCallback(...);
```

### InitGameState Hooks

Intercept game state initialization.

```cpp
using InitGameStateCallback = std::function<void(AGameModeBase* Context)>;

RC::Unreal::Hook::RegisterInitGameStatePreCallback([](AGameModeBase* Context) {});
RC::Unreal::Hook::RegisterInitGameStatePostCallback([](AGameModeBase* Context) {});
```

### ProcessInternal / ProcessLocalScriptFunction Hooks

Intercept Blueprint script execution.

```cpp
using ProcessInternalCallback = std::function<void(UObject* Context, FFrame& Stack, void* RESULT_DECL)>;
using ProcessLocalScriptFunctionCallback = std::function<void(UObject* Context, FFrame& Stack, void* RESULT_DECL)>;

RC::Unreal::Hook::RegisterProcessInternalPreCallback(...);
RC::Unreal::Hook::RegisterProcessInternalPostCallback(...);

RC::Unreal::Hook::RegisterProcessLocalScriptFunctionPreCallback(...);
RC::Unreal::Hook::RegisterProcessLocalScriptFunctionPostCallback(...);
```

### CallFunctionByNameWithArguments Hooks

Intercept function calls by name.

```cpp
using CallFunctionByNameWithArgumentsCallback = std::function<std::pair<bool, bool>(
    UObject* Context,
    const TCHAR* Str,
    FOutputDevice& Ar,
    UObject* Executor,
    bool bForceCallWithNonExec)>;
// Returns: {handled, skip_original_function}

RC::Unreal::Hook::RegisterCallFunctionByNameWithArgumentsPreCallback(...);
RC::Unreal::Hook::RegisterCallFunctionByNameWithArgumentsPostCallback(...);
```

### ULocalPlayer Exec Hooks

Intercept local player command execution.

```cpp
struct ULocalPlayerExecCallbackReturnValue {
    bool UseOriginalReturnValue{true};
    bool NewReturnValue{};
    bool ExecuteOriginalFunction{true};
};

using ULocalPlayerExecCallback = std::function<ULocalPlayerExecCallbackReturnValue(
    ULocalPlayer* Context,
    UWorld* InWorld,
    const TCHAR* Cmd,
    FOutputDevice& Ar)>;

RC::Unreal::Hook::RegisterULocalPlayerExecPreCallback(...);
RC::Unreal::Hook::RegisterULocalPlayerExecPostCallback(...);
```

### Waiting for Required Objects

C++ mods are constructed very early, before Unreal is fully initialized. If your mod needs specific game objects to exist before `on_unreal_init()` fires, you can register them in your constructor:

```cpp
MyMod() : CppUserModBase()
{
    ModName = STR("MyMod");

    // Ensure this object exists before on_unreal_init() is called
    RC::Unreal::Hook::AddRequiredObject(STR("/Script/MyGame.MyImportantClass"));
}
```

UE4SS will wait for all required objects to be constructed before calling `on_unreal_init()` on any mod.

---

## Input System

The input system provides keyboard input handling.

**Header:** `#include <Input/Handler.hpp>` and `#include <Input/KeyDef.hpp>`

### Key Enumeration

Common keys available in `Input::Key`:

| Category | Keys |
|----------|------|
| Letters | `A` - `Z` |
| Numbers | `ZERO` - `NINE` |
| Numpad | `NUM_ZERO` - `NUM_NINE`, `MULTIPLY`, `ADD`, `SUBTRACT`, `DECIMAL`, `DIVIDE` |
| Function | `F1` - `F24` |
| Navigation | `LEFT_ARROW`, `UP_ARROW`, `RIGHT_ARROW`, `DOWN_ARROW`, `PAGE_UP`, `PAGE_DOWN`, `HOME`, `END` |
| Editing | `INS`, `DEL`, `BACKSPACE`, `TAB`, `RETURN`, `SPACE` |
| Mouse | `LEFT_MOUSE_BUTTON`, `RIGHT_MOUSE_BUTTON`, `MIDDLE_MOUSE_BUTTON`, `XBUTTON_ONE`, `XBUTTON_TWO` |
| Modifiers | Use `ModifierKey` enum instead |
| Special | `ESCAPE`, `CAPS_LOCK`, `NUM_LOCK`, `SCROLL_LOCK`, `PAUSE`, `PRINT_SCREEN` |

### Modifier Keys

```cpp
enum ModifierKey : uint8_t
{
    SHIFT = 0x10,
    CONTROL = 0x11,
    ALT = 0x12,
};
```

### Using Modifiers

```cpp
// Create a modifier key combination
Input::ModifierKeys mods = {Input::ModifierKey::CONTROL, Input::ModifierKey::SHIFT};

// Or use the array form
Input::Handler::ModifierKeyArray mod_array = {Input::ModifierKey::CONTROL, Input::ModifierKey::ALT};

// Register with modifiers
register_keydown_event(Input::Key::S, mod_array, []() {
    // Ctrl+Alt+S pressed
});
```

---

## Settings Manager

Access UE4SS settings through `UE4SSProgram::settings_manager`.

```cpp
// Check if debug console is visible
bool visible = RC::UE4SSProgram::settings_manager.Debug.DebugConsoleVisible;
```

---

## Output System

Use the dynamic output system to log messages.

**Header:** `#include <DynamicOutput/DynamicOutput.hpp>`

```cpp
// Basic output
RC::Output::send(STR("Hello, World!\n"));

// Formatted output
RC::Output::send(STR("Value: {}\n"), some_value);
```

---

## String Types

UE4SS uses wide strings on Windows. The `STR()` macro automatically handles string literal conversion.

```cpp
// Use STR() for string literals
StringType my_string = STR("Hello");

// StringType is typically std::wstring on Windows
// StringViewType is std::wstring_view
```

---

## Complete Example

Here's a complete example of a C++ mod that demonstrates multiple features:

```cpp
#include <Mod/CppUserModBase.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/Hooks.hpp>
#include <DynamicOutput/DynamicOutput.hpp>

class ExampleMod : public RC::CppUserModBase
{
public:
    ExampleMod() : CppUserModBase()
    {
        ModName = STR("ExampleMod");
        ModVersion = STR("1.0.0");
        ModDescription = STR("An example C++ mod");
        ModAuthors = STR("Your Name");

        // Register a GUI tab
        register_tab(STR("Example Tab"), [](CppUserModBase* mod) {
            UE4SS_ENABLE_IMGUI()
            ImGui::Text("Hello from ExampleMod!");
            if (ImGui::Button("Click Me"))
            {
                RC::Output::send(STR("Button clicked!\n"));
            }
        });

        // Register a hotkey
        register_keydown_event(Input::Key::F6, []() {
            RC::Output::send(STR("F6 pressed!\n"));
        });
    }

    ~ExampleMod() override = default;

    auto on_unreal_init() -> void override
    {
        RC::Output::send(STR("[ExampleMod] Unreal initialized\n"));

        // Register a ProcessEvent hook
        RC::Unreal::Hook::RegisterProcessEventPostCallback(
            [](RC::Unreal::UObject* Context, RC::Unreal::UFunction* Function, void* Parms) {
                // Handle ProcessEvent calls
            });
    }

    auto on_update() -> void override
    {
        // Called every frame - use sparingly!
    }
};

#define EXAMPLE_MOD_API __declspec(dllexport)
extern "C"
{
    EXAMPLE_MOD_API RC::CppUserModBase* start_mod()
    {
        return new ExampleMod();
    }

    EXAMPLE_MOD_API void uninstall_mod(RC::CppUserModBase* mod)
    {
        delete mod;
    }
}
```

---

## Important Notes

1. **CRT Compatibility**: C++ mods must be compiled with the same C Runtime library version as UE4SS, including the same configuration (Debug/Shipping).

2. **Thread Safety**: Be cautious when accessing Unreal objects from different threads. Execute code in hooks of the game thread when necessary.

3. **Performance**: Avoid heavy processing in `on_update()` and tick hook callbacks as they are called every ue4ss update or UE frame, respectively.

4. **Memory Management**: The mod instance created in `start_mod()` must be deleted in `uninstall_mod()`.

5. **ImGui Context**: Always use `UE4SS_ENABLE_IMGUI()` before using ImGui functions in your mod.

6. **String Types**: Use the `STR()` macro for string literals to ensure proper wide string handling on Windows and cross platform support.
