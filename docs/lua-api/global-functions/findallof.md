# FindAllOf

The `FindAllOf` function will find all non-default instances of the supplied class name.

> This function cannot be used to find non-instances or default instances.

## Parameters

| # | Type    | Information |
|---|---------|-------------|
| 1 | string  | Short name of the class to find instances of |

## Return Value

| # | Type         | Sub Type | Information |
|---|--------------|----------|-------------|
| 1 | nil or table | UObject, UClass, or AActor | nil if no instances were found, otherwise a numerically indexed table of all instances |

## Example
Outputs the name of all objects that inherit from the `Actor` class.
```lua
local ActorInstances = FindAllOf("Actor")
if not ActorInstances then
    print("No instances of 'Actor' were found\n")
else
    for Index, ActorInstance in pairs(ActorInstances) do
        print(string.format("[%d] %s\n", Index, ActorInstance:GetFullName()))
    end
end
```

## Performance

How much this costs depends far more on how broad the class is, and on how you compare names afterwards, than on `FindAllOf` itself. All of it runs on the game thread, so it is a frame hitch rather than background work.

In one level of The Exit 8 — a short game whose playable space is a single looping corridor — `"Actor"` returned 255 instances while `"Object"` returned 23,919. That is roughly 94x, from the same call in the same level. The count tracks how much is loaded rather than how large the map is, so a small game is no guarantee of a small result.

Working through a large result is dominated by name comparison. Measured over 23,919 instances, averaged across 7 rounds:

| Loop body | Total | Per instance |
|-----------|-------|--------------|
| `IsValid()` (baseline for call overhead) | 7.7 ms | 0.32 µs |
| `GetFullName()` compared to a string | 42.1 ms | 1.76 µs |
| `GetFName()` compared to an `FName` | 19.4 ms | 0.81 µs |

**Avoid `GetFullName` in production code.** It builds a full path string for every instance. If you need to match by name, use `GetFName` and construct the `FName` you compare against once, outside the loop — unlike `UObject`, two `FName` values compare correctly with `==`. If the leaf name is not specific enough on its own, walk the outer chain and compare `FName`s along it.

```lua
-- Construct the FName once, not per instance.
local Target = FName("BP_SomeActor_C_0")
for _, Instance in pairs(FindAllOf("Object") or {}) do
    if Instance:GetFName() == Target then
        -- Only now is it worth building the full path, if you need it at all.
    end
end
```

If you look the same objects up repeatedly, cache them rather than re-scanning.

> Figures above are from The Exit 8 (UE 5.2) with UE4SS **v3.0.1 Beta #0, Git SHA `d7e7826d`** — an experimental build, not 3.0.1 stable — on a Ryzen 9 9950X3D. They are meant to show the shape of the cost rather than to be exact for any other title, engine version, or UE4SS build.
