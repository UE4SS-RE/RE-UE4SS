#pragma once

#include <chrono>
#include <filesystem>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include <Mod/CppUserModBase.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include "MMA/Types.hpp"
#include "MMA/DataTable.hpp"

namespace RC::Unreal { struct FFrame; }

namespace RC::FF7MMA {

    /**
     * FF7MMAMod - FF7 Rebirth memory mapped asset (EndDataObject) support.
     *
     * On startup it periodically scans for loaded MMA objects and dumps the ones
     * listed in ue4ss/MMADumps/targets.txt (or all, if the file is absent) to
     * ue4ss/MMADumps/<name>.json, writing status.json and a COMPLETE marker so an
     * external harness can drive iteration.
     */
    class FF7MMAMod : public CppUserModBase {
    public:
        FF7MMAMod();
        ~FF7MMAMod() override;

        auto on_unreal_init() -> void override;
        auto on_update() -> void override;

        static FF7MMAMod* GetInstance() { return s_instance; }

        std::vector<Unreal::UObject*> FindAllMMAObjects();
        MMADataTable* GetParsedTable(Unreal::UObject* mmaObject);

        // Applies a data-mod JSON document to the live tables immediately and
        // returns a result summary. Game thread only. Used by the Blueprint
        // bridge; other C++ mods can call it too.
        std::string ApplyJsonText(const std::string& jsonText);
        // Re-reads data/*.json, forgets which tables were already patched and
        // applies the new set to the live tables. Additive: a value a file no
        // longer sets keeps its last applied value until restart. Locks
        // m_opMutex, so it is safe from the game thread or a queued op.
        std::string ReloadDataMods();
        bool IsMMAObject(Unreal::UObject* object);
        // True for EndDataObjectBase-derived assets (the parseable data tables);
        // animations and other MMA subtypes are not tables
        bool IsDataObject(Unreal::UObject* object);

    private:
        void CacheMMABaseClass();
        void AutoDumpTick();
        void WriteStatus();
        void LoadDataFiles();
        void ApplyTableMods(Unreal::UObject* obj, const std::string& name, const std::string& lowerName);

        // Rebuilds the table into a fresh frozen image, verifies it and swaps the
        // asset's content pointer. Shared by the JSON pipeline and the UI editor.
        bool RebuildAndSwap(Unreal::UObject* obj, MMADataTable& table, std::string& outMessage);
        bool RestoreOriginal(Unreal::UObject* obj);

        // The ImGui tab runs on the render thread; anything that creates FNames or
        // writes live table memory is queued and executed on the game thread.
        struct PendingOp {
            enum class Kind { NameEdit, AddRow, Apply, Restore, PatchValue, PatchString, PatchArray,
                              CloneDO, NewEmptyDO, ReloadJson };
            Kind OpKind{};
            Unreal::UObject* Object = nullptr;
            size_t RowIndex = 0;
            size_t PropIndex = 0;
            size_t ElemIndex = SIZE_MAX; // set for array elements
            std::string Text;            // new name text, or new row name
            std::string CloneFrom;
            PropertyValue Value;         // for PatchValue
        };
        void QueueOp(PendingOp op);
        void ProcessPendingOps();
        void CheckReloadTrigger();

        // In-place live-blob patching (game thread only). These write into the
        // memory the game is actually reading; the parsed copy stays the mirror.
        bool PatchScalarLive(Unreal::UObject* obj, MMADataTable& table, size_t rowIdx, size_t propIdx,
                             const PropertyValue& value, std::string& outMessage);
        bool PatchStringLive(Unreal::UObject* obj, MMADataTable& table, size_t rowIdx, size_t propIdx,
                             std::string& outMessage);
        bool PatchArrayLive(Unreal::UObject* obj, MMADataTable& table, size_t rowIdx, size_t propIdx,
                            std::string& outMessage);

        // Constructs a brand-new data object of the source's class (game thread).
        // copyRows keeps the source's rows; otherwise the new table is empty with
        // the same schema. The frozen image is built by us and fully editable.
        Unreal::UObject* CreateDataObject(Unreal::UObject* source, const std::string& newName,
                                          bool copyRows, std::string& outMessage);

        // Core apply pipeline for one table object; rowEntries is a ModJson row map
        std::string ApplyEntriesToObject(Unreal::UObject* obj, const void* rowEntries);

        // Blueprint bridge: functions named MMA_* in loaded blueprints are handled
        // by us, and completion events are fired back into the FF7RML game instance
        void SetupBlueprintBridge();
        void OnScriptFunctionPre(Unreal::FFrame& stack);
        void QueueBpTableEvent(const std::string& table, const std::string& summary);
        void FireBpEvents();

        std::vector<std::pair<std::string, std::string>> m_pendingBpEvents;
        std::string m_reloadStatus;
        std::chrono::steady_clock::time_point m_lastTriggerCheck{};
        bool m_allModsEventFired = false;
        bool m_readyEventFired = false;
        void RenderUI();
        void RenderTableView(Unreal::UObject* obj, MMADataTable* table);

        // Reads the unreflected UMemoryMappedAsset fields
        struct MMAFields {
            Unreal::FTypeLayoutDesc* Layout = nullptr;
            uint32_t SomeBools = 0;
            uint32_t FrozenContentSize = 0;
            uint8_t* Content = nullptr;
        };
        static MMAFields ReadMMAFields(Unreal::UObject* object);

        std::unordered_map<Unreal::UObject*, std::unique_ptr<MMADataTable>> m_parsedTables;

        // Auto-dump state
        bool m_unrealInitialized = false;
        bool m_autoDumpDone = false;
        std::filesystem::path m_dumpDir;
        std::set<std::string> m_targets;          // lowercase names; empty = dump everything found
        std::map<std::string, std::string> m_dumped;  // name -> file
        std::map<std::string, Unreal::UObject*> m_dumpedFrom; // instance a dump came from
        std::map<std::string, std::string> m_failed;  // name -> error
        std::set<std::string> m_seen;
        std::chrono::steady_clock::time_point m_startTime{};
        std::chrono::steady_clock::time_point m_lastScan{};

        // UI state. The object list is a refresh-time snapshot: names and type
        // flags are captured once so rendering never dereferences stale pointers.
        struct MMAListEntry {
            Unreal::UObject* Object;
            std::string Name;
            bool bIsDataObject;
        };
        std::vector<MMAListEntry> m_mmaObjects;
        Unreal::UObject* m_selectedObject = nullptr;
        char m_searchBuffer[256] = {};
        char m_rowFilter[256] = {};
        char m_newRowName[128] = {};
        char m_cloneSource[128] = {};
        char m_newDOName[128] = {};
        bool m_needsRefresh = true;
        bool m_showAllMMA = false;
        std::set<Unreal::UObject*> m_dirtyTables;
        std::map<Unreal::UObject*, std::string> m_applyStatus;

        // Guards m_parsedTables, m_dirtyTables, m_applyStatus and m_pendingOps
        // against render-thread / game-thread races. Recursive because the UI holds
        // it across the details pane while QueueOp locks it again.
        std::recursive_mutex m_opMutex;
        std::vector<PendingOp> m_pendingOps;

        static FF7MMAMod* s_instance;
        static Unreal::UClass* s_mmaBaseClass;
        static Unreal::UClass* s_dataObjectBaseClass;
        static bool s_mmaClassSearched;
    };

} // namespace RC::FF7MMA
