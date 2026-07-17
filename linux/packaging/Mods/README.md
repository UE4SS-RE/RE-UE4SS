# Linux mods directory

The native Linux bootstrap discovers enabled Lua mods and executes each one in
an isolated VM only after the Unreal resolver, ABI, object-registry, and Lua
binding gates pass. The current MVP includes UObject discovery and identity,
scalar reflected properties, game-thread UFunction calls, scheduling,
native/Blueprint `RegisterHook`, scoped hook parameters, and
`NotifyOnNewObject`. Unsupported GUI, input, complex container/struct, and C++
mod features remain fail-closed.

The eventual canonical Lua layout is:

```text
Mods/<ModName>/enabled.txt
Mods/<ModName>/Scripts/main.lua
```

Names are resolved with Windows-compatible ASCII case-insensitive semantics.
Ambiguous variants such as `Scripts` and `scripts`, case-colliding mod names,
and symlinked mod content are rejected instead of being guessed.

`LinuxReadOnlyConformance` is shipped disabled. Rename its
`enabled.example` to `enabled.txt` only for an explicit compatibility test.
