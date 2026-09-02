# FF7MMAMod

UE4SS C++ mod for editing FF7 Rebirth's data tables (EndDataObject assets like CharaSpec, BattleAbility, Materia) at runtime. Edits apply to the running game, no repacking involved.

## JSON data mods

Drop `.json` files into `ue4ss/Mods/FF7MMAMod/data/` next to the game binary. Files load alphabetically and merge per property: two mods can change different properties of the same row and both apply. If they set the same property, the later file wins. A `$delete` replaces earlier edits for that row. The top level maps table names to row objects:

```json
{
  "CharaSpec": {
    "PC0000_00_Cloud_Standard": { "AnimationPlayRate": 1.5 },
    "MyNewCloud_00": {
      "$clone": "PC0000_00_Cloud_Standard",
      "CharaID": "MYCLOUD",
      "AnimationSetList_Array": ["Battle00", "Neutral00"]
    },
    "SomeRowToRemove": { "$delete": true }
  },
  "BattleAbility": {
    "PC0000_NAtk_1": { "ATB": 7.25 }
  }
}
```

- Existing row names edit that row. Unknown names add a new row.
- Row names containing `*` are wildcards and edit every matching row, for example `"PC0000_*": { "ATB": 5 }`. A wildcard set to `null` or containing `$delete` deletes the matching rows. Exact names are applied after wildcards and win.
- `"$all": { ... }` edits every row in the table. Wildcards and named entries are applied after it.
- `$clone` copies all values from an existing row before your overrides. Without it a new row starts with everything zeroed or empty.
- `$delete: true` or setting the row to `null` removes it. `$priority` sets the key priority of a new row.
- `*_Array` properties take JSON arrays.

Results are written to `ue4ss/MMADumps/status.json` and the UE4SS log.

Easier than writing these by hand:

- **Templates**: every dumped table also gets `ue4ss/MMADumps/templates/<Table>.template.json` (or use the "Export Template" button). It is a loadable data-mod file with a `$schema` block listing every property and type, plus one real example row. Copy it and delete what you do not want to change.
- **Session export**: edit live in the "FF7 MMA" tab, then "Export Mod JSON" writes a file to `ue4ss/MMADumps/exports/` containing only what you changed. Copy it into `data/` to make it permanent.
- **Offline editor**: open `tools/mma_mod_editor.html` in a browser. It has the schema for every table built in, so "New table" starts a mod from scratch without the game running (`tools/embed_schemas.py` refreshes the schemas from `MMADumps/schemas.json` after a game update). It also opens dumps and templates for editing real data, with row filtering by name, edit status or property values, sorting, bulk edits across filtered rows, and export of the finished mod JSON.

## Adding new rows

A row entry whose name does not exist yet adds that row:

```json
{
  "CharaSpec": {
    "ZZMyChara_00": {
      "$clone": "PC0000_00_Cloud_Standard",
      "CharaID": "ZZMYCHARA",
      "AnimationPlayRate": 1.0
    }
  }
}
```

Clone the closest existing row and change only what you need. A new row shows up in game once something references it, such as a placement entry or another table's Name property, so new content usually means touching a few tables in the same file. JSON adds apply at load. In the "FF7 MMA" tab of the UE4SS GUI, row adds and deletes take effect on "Apply Changes".

## Creating new data objects

A table entry whose name matches no loaded object, plus `$create` naming the source table, creates a whole new table of that class:

```json
{
  "ZZMyTable": {
    "$create": "CharaSpec",
    "MyRow_00": { "CharaID": "MYID", "AnimationPlayRate": 1.0 }
  }
}
```

The new table gets the source's schema and the listed rows, and behaves like any other table in the tab and dumps. The "Clone DO" button in the tab does the same with a full copy of the source's rows, "New Empty DO" with no rows.

## In-game editor

The "FF7 MMA" tab in the UE4SS GUI lists the loaded data tables (tick "Show all MMA" to also see animations and other memory mapped assets, which are not tables). Pick a table for a filterable row browser with editors for every property, including array members. Value, string and array edits apply to the live game immediately. Rows can be added, cloned and deleted, committed with "Apply Changes". "Restore Original" reverts a table's row changes. "Export JSON", "Export Template" and "Export Mod JSON" write to `ue4ss/MMADumps/`. "Reload JSON" re-reads `data/` and applies it again without restarting, and fires the completion events again. It is additive: a value you remove from a file keeps its last applied value until the game restarts. Creating an empty `ue4ss/MMADumps/reload.trigger` file does the same from outside the game (it is deleted once picked up), for editor or script integration.

## Blueprint API

Blueprint mods (UMG menus and the like) can drive edits at runtime through stub functions that this mod intercepts by name. Create functions with these exact names and parameter names in any loaded blueprint, give them empty bodies, and call them normally. Every parameter and return is a plain String (FString) unless noted; parameters named with Json carry JSON as text, so no JSON plugin or struct types are involved:

- `MMA_ApplyJson(Json) -> Result`: applies a full data-mod JSON document (same format as the files, including `$` directives and wildcards).
- `MMA_ReloadJson() -> Result`: re-reads `data/` and applies it again, same as the "Reload JSON" button.
- `MMA_Set(Table, Row, Prop, Value) -> Result`: single value edit. Declaring Value as String covers every property type, since the text is parsed into the property's actual type (numbers, true/false, names). Value can also be declared as Float, Integer, Integer64, Boolean or Name for direct pins, and stubs with differently typed Value pins can coexist in different blueprints since matching is by function name.
- `MMA_SetValue(Table, Row, Prop, ValueJson) -> Result`: single value edit with the value as JSON text (needed for arrays).
- `MMA_GetValue(Table, Row, Prop) -> ValueJson`: reads one value as JSON.
- `MMA_GetRow(Table, Row) -> RowJson`: reads a whole row as a JSON object.
- `MMA_GetSchema(Table) -> SchemaJson`: the table's properties as a JSON array of name, type and isArray.

Three optional events can be defined on `/FF7RML/BP_EndGameInstance.BP_EndGameInstance_C`. `MMA_OnReady()` fires once UE4SS and this mod are initialised and the `MMA_*` stubs are being intercepted, so a blueprint loader should hold its `MMA_*` calls until then (calls made earlier just run the stub's empty body). `MMA_OnTableModsApplied(Table, Summary)` fires after mods apply to a table, including after a reload. `MMA_OnAllModsApplied()` fires once every staged mod file has been applied.

## Table dumps

Dumping is opt-in. Tables listed in `ue4ss/MMADumps/targets.txt` (one name per line) are dumped at startup to `ue4ss/MMADumps/<name>.json` with keys, schema, hash buckets and all row values; a `*` line dumps every data table. Without the file nothing is dumped (JSON mods still apply). A `COMPLETE` marker is written when all listed targets are done, and dumps reflect any applied JSON mods.
