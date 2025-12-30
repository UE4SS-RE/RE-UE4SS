#include <GUI/Profilers.hpp>

#ifdef UE4SS_PROFILER_TAB

#include <chrono>
#include <filesystem>

#include <DynamicOutput/DynamicOutput.hpp>
#include <ExceptionHandling.hpp>
#include <Constructs/Loop.hpp>
#include <UE4SSProgram.hpp>
#include <Unreal/AActor.hpp>
#include <Unreal/CoreUObject/UObject/Class.hpp>
#include <Unreal/Searcher/ObjectSearcher.hpp>
#include <Unreal/Searcher/ObjectSearcherProfiler.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/FOutputDevice.hpp>
#include <Unreal/UObjectHashTables.hpp>

#include <imgui.h>

namespace RC::GUI::Profilers
{
    using namespace Unreal;

    auto render() -> void
    {
        ImGui::Text("Object Searcher Pool Profiler");
        ImGui::Separator();

        bool profiling_enabled = ObjectSearcherProfiler::IsProfilingEnabled();
        if (ImGui::Checkbox("Enable Profiling", &profiling_enabled))
        {
            ObjectSearcherProfiler::EnableProfiling(profiling_enabled);
        }

        if (ImGui::Button("Reset Stats"))
        {
            ObjectSearcherProfiler::ResetAllStats();
        }

        ImGui::SameLine();
        if (ImGui::Button("Dump to File"))
        {
            TRY([] {
                std::filesystem::path profiler_dir = std::filesystem::path(UE4SSProgram::get_program().get_working_directory()) / "ProfilerDumps";
                if (ObjectSearcherProfiler::DumpStatsToFile(profiler_dir))
                {
                    Output::send(STR("Profiler stats dumped to {}\n"), profiler_dir.wstring());
                }
                else
                {
                    Output::send<LogLevel::Warning>(STR("Failed to dump profiler stats\n"));
                }
            });
        }

        ImGui::SameLine();
        if (ImGui::Button("Snapshot Pool Sizes"))
        {
            ObjectSearcherProfiler::FastStats.LastPoolSizeActors.store(
                    ObjectSearcherPool<AActor, AnySuperStruct>::Pool.size(),
                    std::memory_order_relaxed);
            ObjectSearcherProfiler::FastStats.LastPoolSizeClasses.store(
                    ObjectSearcherPool<UClass, AnySuperStruct>::Pool.size(),
                    std::memory_order_relaxed);
            ObjectSearcherProfiler::FastStats.LastPoolSizeActorClasses.store(
                    ObjectSearcherPool<UClass, AActor>::Pool.size(),
                    std::memory_order_relaxed);
            ObjectSearcherProfiler::FastStats.LastPoolSizeInstances.store(
                    ObjectSearcherPool<AnyInstance, AnyInstance>::Pool.size(),
                    std::memory_order_relaxed);
        }

        ImGui::Text("Pool Sizes: Actors=%llu, Classes=%llu, ActorClasses=%llu, Instances=%llu",
                    ObjectSearcherProfiler::FastStats.LastPoolSizeActors.load(std::memory_order_relaxed),
                    ObjectSearcherProfiler::FastStats.LastPoolSizeClasses.load(std::memory_order_relaxed),
                    ObjectSearcherProfiler::FastStats.LastPoolSizeActorClasses.load(std::memory_order_relaxed),
                    ObjectSearcherProfiler::FastStats.LastPoolSizeInstances.load(std::memory_order_relaxed));

        // Fast path stats
        ImGui::Spacing();
        ImGui::Text("=== Fast Path (Pool) ===");
        const auto& fast = ObjectSearcherProfiler::FastStats;
        ImGui::Text("Add:    %llu calls, avg %.3f us (sampled 1:%llu), total %.2f ms",
                    fast.AddCount.load(std::memory_order_relaxed),
                    fast.AvgAddTimeUs(kAddRemoveSampleRate),
                    kAddRemoveSampleRate,
                    fast.TotalAddTimeMs());
        ImGui::Text("Remove: %llu calls, avg %.3f us (sampled 1:%llu), total %.2f ms",
                    fast.RemoveCount.load(std::memory_order_relaxed),
                    fast.AvgRemoveTimeUs(kAddRemoveSampleRate),
                    kAddRemoveSampleRate,
                    fast.TotalRemoveTimeMs());
        ImGui::Text("Search: %llu calls, avg %.3f us, total %.2f ms",
                    fast.SearchCount.load(std::memory_order_relaxed),
                    fast.AvgSearchTimeUs(),
                    fast.TotalSearchTimeMs());

        if (ImGui::Button("Test: FindAllOf<AActor>"))
        {
            TRY([] {
                size_t count = 0;
                FindObjectSearcher<AActor, AnySuperStruct>().ForEach([&](UObject*) {
                    ++count;
                    return LoopAction::Continue;
                });
                Output::send(STR("Fast search found {} actors\n"), count);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Test: FindAllOf<UClass>"))
        {
            TRY([] {
                size_t count = 0;
                FindObjectSearcher<UClass, AnySuperStruct>().ForEach([&](UObject*) {
                    ++count;
                    return LoopAction::Continue;
                });
                Output::send(STR("Fast search found {} classes\n"), count);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Test: Fast Instances"))
        {
            TRY([] {
                std::vector<UObject*> Instances{};
                UObjectGlobals::FindAllOf(STR("Actor"), Instances, true);
                Output::send(STR("Fast search found {} instances (Actor)\n"), Instances.size());
            });
        }

        // Slow path stats
        ImGui::Spacing();
        ImGui::Text("=== Slow Path (Full Scan) ===");
        const auto& slow = ObjectSearcherProfiler::SlowStats;
        ImGui::Text("Search: %llu calls, avg %.3f us, total %.2f ms",
                    slow.SearchCount.load(std::memory_order_relaxed),
                    slow.AvgSearchTimeUs(),
                    slow.TotalSearchTimeMs());

        if (ImGui::Button("Test: Slow AActor"))
        {
            TRY([] {
                size_t count = 0;
                ObjectSearcherSlowInternal(AActor::StaticClass(),
                                           nullptr,
                                           [&](UObject*) {
                                               ++count;
                                               return LoopAction::Continue;
                                           },
                                           nullptr);
                Output::send(STR("Slow search found {} actors\n"), count);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Test: Slow UClass"))
        {
            TRY([] {
                size_t count = 0;
                ObjectSearcherSlowInternal(UClass::StaticClass(),
                                           nullptr,
                                           [&](UObject*) {
                                               ++count;
                                               return LoopAction::Continue;
                                           },
                                           nullptr);
                Output::send(STR("Slow search found {} classes\n"), count);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Test: Slow Instances"))
        {
            TRY([] {
                std::vector<UObject*> Instances{};
                UObjectGlobals::FindAllOf(STR("Actor"), Instances, false);
                Output::send(STR("Slow search found {} instances (Actor)\n"), Instances.size());
            });
        }

        // GUObjectArray iteration stats
        ImGui::Spacing();
        ImGui::Text("=== GUObjectArray Iteration ===");
        const auto& guobjStats = ObjectSearcherProfiler::GUObjectArrayIterStats;
        ImGui::Text("Iterations: %llu calls, avg %.3f us, total %.2f ms",
                    guobjStats.IterationCount.load(std::memory_order_relaxed),
                    guobjStats.AvgIterationTimeUs(),
                    guobjStats.TotalIterationTimeMs());
        ImGui::Text("Objects iterated: %llu total, avg %.0f per iteration",
                    guobjStats.ObjectsIterated.load(std::memory_order_relaxed),
                    guobjStats.AvgObjectsPerIteration());

        if (ImGui::Button("Test: ForEachUObject"))
        {
            TRY([] {
                size_t count = 0;
                UObjectGlobals::ForEachUObject([&](UObject*, int32, int32) {
                    ++count;
                    return LoopAction::Continue;
                });
                Output::send(STR("ForEachUObject iterated {} objects\n"), count);
            });
        }

        // FUObjectHashTables stats
        ImGui::Spacing();
        ImGui::Text("=== FUObjectHashTables (ClassToObjectListMap) ===");

        bool hashTablesAvailable = FUObjectHashTables::IsAvailable();
        ImGui::Text("Status: %s", hashTablesAvailable ? "Available" : "NOT AVAILABLE (signature not found)");

        if (!hashTablesAvailable)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), "HashTable tests disabled - signature not found");
        }
        else
        {
            // Debug: show map info
            if (ImGui::Button("Debug: Show Map Info"))
            {
                TRY([] {
                    auto& Tables = FUObjectHashTables::Get();
                    auto& Map = Tables.GetClassToObjectListMap();
                    Output::send(STR("ClassToObjectListMap: Num={}\n"), Map.Num());

                    // Try iterating all entries
                    int count = 0;
                    UClass* foundActorClass = nullptr;
                    for (auto& Pair : Map)
                    {
                        ++count;
                        if (count <= 5)
                        {
                            Output::send(STR("  Entry {}: Class={}, Bucket.Num={}\n"),
                                         count,
                                         Pair.Key ? Pair.Key->GetName() : STR("null"),
                                         Pair.Value.Num());
                        }
                        // Check if this is AActor class
                        if (Pair.Key == AActor::StaticClass())
                        {
                            foundActorClass = Pair.Key;
                        }
                    }
                    Output::send(STR("Total entries iterated: {}\n"), count);

                    // Now try Find with the same class
                    auto* ActorClass = AActor::StaticClass();
                    Output::send(STR("AActor::StaticClass() = {}\n"), static_cast<void*>(ActorClass));
                    Output::send(STR("Found AActor in iteration: {}\n"), foundActorClass ? STR("yes") : STR("no"));

                    auto* Bucket = Map.Find(ActorClass);
                    Output::send(STR("Map.Find(AActor) = {}\n"), Bucket ? STR("found") : STR("nullptr"));
                });
            }
        }

        const auto& hashStats = ObjectSearcherProfiler::HashTableStats;
        ImGui::Text("Search: %llu calls, avg %.3f us, total %.2f ms",
                    hashStats.SearchCount.load(std::memory_order_relaxed),
                    hashStats.AvgSearchTimeUs(),
                    hashStats.TotalSearchTimeMs());
        ImGui::Text("Objects: %llu found, avg %.1f per search",
                    hashStats.ObjectsFound.load(std::memory_order_relaxed),
                    hashStats.AvgObjectsPerSearch());
        ImGui::Text("Buckets: %llu hits, %llu misses (%.1f%% hit rate)",
                    hashStats.BucketHits.load(std::memory_order_relaxed),
                    hashStats.BucketMisses.load(std::memory_order_relaxed),
                    hashStats.BucketHitRate());

        if (ImGui::Button("Test: HashTable AActor"))
        {
            TRY([] {
                HASHTABLE_PROFILE_SEARCH_BEGIN()
                size_t count = 0;
                auto* ActorClass = AActor::StaticClass();
                const FHashBucket* Bucket = FUObjectHashTables::Get().FindClassBucket(ActorClass);
                if (Bucket)
                {
                    HASHTABLE_PROFILE_BUCKET_HIT()
                    Bucket->ForEach([&](UObjectBase* Obj) {
                        HASHTABLE_PROFILE_OBJ_COUNT()
                        ++count;
                    });
                }
                else
                {
                    HASHTABLE_PROFILE_BUCKET_MISS()
                }
                HASHTABLE_PROFILE_SEARCH_END()
                Output::send(STR("HashTable search found {} actors\n"), count);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Test: HashTable UClass"))
        {
            TRY([] {
                HASHTABLE_PROFILE_SEARCH_BEGIN()
                size_t count = 0;
                auto* ClassClass = UClass::StaticClass();
                const FHashBucket* Bucket = FUObjectHashTables::Get().FindClassBucket(ClassClass);
                if (Bucket)
                {
                    HASHTABLE_PROFILE_BUCKET_HIT()
                    Bucket->ForEach([&](UObjectBase* Obj) {
                        HASHTABLE_PROFILE_OBJ_COUNT()
                        ++count;
                    });
                }
                else
                {
                    HASHTABLE_PROFILE_BUCKET_MISS()
                }
                HASHTABLE_PROFILE_SEARCH_END()
                Output::send(STR("HashTable search found {} classes\n"), count);
            });
        }

        // Direct comparison section
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("=== Direct Comparison (HashTable vs Slow) ===");

        // Helper lambda for running comparison
        auto RunComparison = [](const wchar_t* className, UClass* classPtr, int iterations) {
            if (!classPtr)
            {
                Output::send<LogLevel::Warning>(STR("Class {} not found\n"), className);
                return;
            }

            // HashTable method (including derived classes)
            auto hashStart = std::chrono::high_resolution_clock::now();
            size_t hashCount = 0;
            for (int i = 0; i < iterations; ++i)
            {
                FUObjectHashTables::Get().ForEachObjectOfClassIncludingDerived(classPtr, [&](UObjectBase* Obj) {
                    ++hashCount;
                });
            }
            auto hashEnd = std::chrono::high_resolution_clock::now();
            auto hashTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(hashEnd - hashStart).count();

            // Slow method (GUObjectArray scan)
            auto slowStart = std::chrono::high_resolution_clock::now();
            size_t slowCount = 0;
            for (int i = 0; i < iterations; ++i)
            {
                ObjectSearcherSlowInternal(classPtr,
                                           nullptr,
                                           [&](UObject*) {
                                               ++slowCount;
                                               return LoopAction::Continue;
                                           },
                                           nullptr);
            }
            auto slowEnd = std::chrono::high_resolution_clock::now();
            auto slowTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(slowEnd - slowStart).count();

            Output::send(STR("=== {} ({} iterations) ===\n"), className, iterations);
            Output::send(STR("HashTable: {} objects, {} us total, {:.2f} us/iter\n"),
                         hashCount / iterations, hashTimeUs, static_cast<double>(hashTimeUs) / iterations);
            Output::send(STR("Slow:      {} objects, {} us total, {:.2f} us/iter\n"),
                         slowCount / iterations, slowTimeUs, static_cast<double>(slowTimeUs) / iterations);

            if (hashTimeUs > 0 && slowTimeUs > 0)
            {
                Output::send(STR("HashTable is {:.1f}x faster than Slow\n"),
                             static_cast<double>(slowTimeUs) / hashTimeUs);
            }
            Output::send(STR("\n"));
        };

        if (ImGui::Button("Compare AActor x100"))
        {
            TRY([&RunComparison] {
                RunComparison(STR("AActor"), AActor::StaticClass(), 100);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Compare UClass x100"))
        {
            TRY([&RunComparison] {
                RunComparison(STR("UClass"), UClass::StaticClass(), 100);
            });
        }

        if (ImGui::Button("Compare PlayerController x100"))
        {
            TRY([&RunComparison] {
                auto* PlayerControllerClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.PlayerController"));
                RunComparison(STR("PlayerController"), PlayerControllerClass, 100);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Compare Pawn x100"))
        {
            TRY([&RunComparison] {
                auto* PawnClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.Pawn"));
                RunComparison(STR("Pawn"), PawnClass, 100);
            });
        }

        if (ImGui::Button("Run All Comparisons"))
        {
            TRY([&RunComparison] {
                Output::send(STR("========== FULL COMPARISON ==========\n\n"));

                RunComparison(STR("AActor"), AActor::StaticClass(), 100);
                RunComparison(STR("UClass"), UClass::StaticClass(), 100);

                auto* PlayerControllerClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.PlayerController"));
                if (PlayerControllerClass)
                    RunComparison(STR("PlayerController"), PlayerControllerClass, 100);

                auto* PawnClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.Pawn"));
                if (PawnClass)
                    RunComparison(STR("Pawn"), PawnClass, 100);

                auto* CharacterClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.Character"));
                if (CharacterClass)
                    RunComparison(STR("Character"), CharacterClass, 100);

                auto* ActorComponentClass = UObjectGlobals::StaticFindObject<UClass*>(nullptr, nullptr, STR("/Script/Engine.ActorComponent"));
                if (ActorComponentClass)
                    RunComparison(STR("ActorComponent"), ActorComponentClass, 100);

                Output::send(STR("========== END COMPARISON ==========\n"));
            });
        }

        // StaticFindObject comparison section
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Text("=== StaticFindObject (HashTable vs GUObjectArray) ===");

        // Helper lambda for StaticFindObject comparison with outer
        auto RunStaticFindObjectWithOuterComparison = [](const wchar_t* label, UClass* objectClass, UObject* outer, FName objectName, int iterations) {
            // Hash table method
            auto hashStart = std::chrono::high_resolution_clock::now();
            UObject* hashResult = nullptr;
            for (int i = 0; i < iterations; ++i)
            {
                hashResult = FUObjectHashTables::StaticFindObject(objectClass, outer, objectName, false, true);
            }
            auto hashEnd = std::chrono::high_resolution_clock::now();
            auto hashTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(hashEnd - hashStart).count();

            // Slow method (GUObjectArray scan)
            auto slowStart = std::chrono::high_resolution_clock::now();
            UObject* slowResult = nullptr;
            for (int i = 0; i < iterations; ++i)
            {
                slowResult = FUObjectHashTables::StaticFindObject(objectClass, outer, objectName, false, false);
            }
            auto slowEnd = std::chrono::high_resolution_clock::now();
            auto slowTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(slowEnd - slowStart).count();

            Output::send(STR("=== {} ({} iterations) ===\n"), label, iterations);
            Output::send(STR("  Class: {}, Outer: {}, Name: {}\n"),
                         objectClass ? objectClass->GetName() : STR("any"),
                         outer ? outer->GetName() : STR("nullptr (top-level)"),
                         objectName.IsNone() ? STR("None") : objectName.ToString());
            Output::send(STR("HashTable: {} us total, {:.2f} us/iter, found: {}\n"),
                         hashTimeUs, static_cast<double>(hashTimeUs) / iterations,
                         hashResult ? hashResult->GetFullName() : STR("nullptr"));
            Output::send(STR("Slow:      {} us total, {:.2f} us/iter, found: {}\n"),
                         slowTimeUs, static_cast<double>(slowTimeUs) / iterations,
                         slowResult ? slowResult->GetFullName() : STR("nullptr"));

            if (hashTimeUs > 0 && slowTimeUs > 0)
            {
                Output::send(STR("HashTable is {:.1f}x faster than Slow\n"),
                             static_cast<double>(slowTimeUs) / hashTimeUs);
            }

            // Verify results match
            if (hashResult != slowResult)
            {
                Output::send<LogLevel::Warning>(STR("WARNING: Results don't match! HashTable={}, Slow={}\n"),
                                                 hashResult ? hashResult->GetFullName() : STR("nullptr"),
                                                 slowResult ? slowResult->GetFullName() : STR("nullptr"));
            }
            Output::send(STR("\n"));
        };

        // Test finding top-level packages (no outer)
        ImGui::Text("Top-level packages (no outer):");
        if (ImGui::Button("Find '/Script/Engine' x1000"))
        {
            TRY([&RunStaticFindObjectWithOuterComparison] {
                RunStaticFindObjectWithOuterComparison(STR("Package /Script/Engine"), nullptr, nullptr, FName(STR("/Script/Engine")), 1000);
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Find '/Script/CoreUObject' x1000"))
        {
            TRY([&RunStaticFindObjectWithOuterComparison] {
                RunStaticFindObjectWithOuterComparison(STR("Package /Script/CoreUObject"), nullptr, nullptr, FName(STR("/Script/CoreUObject")), 1000);
            });
        }

        // Test finding objects with outer (e.g., PlayerController class inside /Script/Engine package)
        ImGui::Text("Objects with outer (class inside package):");
        if (ImGui::Button("Find PlayerController in /Script/Engine x1000"))
        {
            TRY([&RunStaticFindObjectWithOuterComparison] {
                // First find the /Script/Engine package
                UObject* EnginePackage = FUObjectHashTables::StaticFindObject(nullptr, nullptr, FName(STR("/Script/Engine")), false, true);
                if (EnginePackage)
                {
                    RunStaticFindObjectWithOuterComparison(STR("PlayerController in /Script/Engine"),
                                                            nullptr, EnginePackage, FName(STR("PlayerController")), 1000);
                }
                else
                {
                    Output::send<LogLevel::Warning>(STR("Could not find /Script/Engine package\n"));
                }
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Find Actor in /Script/Engine x1000"))
        {
            TRY([&RunStaticFindObjectWithOuterComparison] {
                UObject* EnginePackage = FUObjectHashTables::StaticFindObject(nullptr, nullptr, FName(STR("/Script/Engine")), false, true);
                if (EnginePackage)
                {
                    RunStaticFindObjectWithOuterComparison(STR("Actor in /Script/Engine"),
                                                            nullptr, EnginePackage, FName(STR("Actor")), 1000);
                }
                else
                {
                    Output::send<LogLevel::Warning>(STR("Could not find /Script/Engine package\n"));
                }
            });
        }

        if (ImGui::Button("Find Pawn in /Script/Engine x1000"))
        {
            TRY([&RunStaticFindObjectWithOuterComparison] {
                UObject* EnginePackage = FUObjectHashTables::StaticFindObject(nullptr, nullptr, FName(STR("/Script/Engine")), false, true);
                if (EnginePackage)
                {
                    RunStaticFindObjectWithOuterComparison(STR("Pawn in /Script/Engine"),
                                                            nullptr, EnginePackage, FName(STR("Pawn")), 1000);
                }
                else
                {
                    Output::send<LogLevel::Warning>(STR("Could not find /Script/Engine package\n"));
                }
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Find Character in /Script/Engine x1000"))
        {
            TRY([&RunStaticFindObjectWithOuterComparison] {
                UObject* EnginePackage = FUObjectHashTables::StaticFindObject(nullptr, nullptr, FName(STR("/Script/Engine")), false, true);
                if (EnginePackage)
                {
                    RunStaticFindObjectWithOuterComparison(STR("Character in /Script/Engine"),
                                                            nullptr, EnginePackage, FName(STR("Character")), 1000);
                }
                else
                {
                    Output::send<LogLevel::Warning>(STR("Could not find /Script/Engine package\n"));
                }
            });
        }

        // Find UObject in CoreUObject
        if (ImGui::Button("Find Object in /Script/CoreUObject x1000"))
        {
            TRY([&RunStaticFindObjectWithOuterComparison] {
                UObject* CoreUObjectPackage = FUObjectHashTables::StaticFindObject(nullptr, nullptr, FName(STR("/Script/CoreUObject")), false, true);
                if (CoreUObjectPackage)
                {
                    RunStaticFindObjectWithOuterComparison(STR("Object in /Script/CoreUObject"),
                                                            nullptr, CoreUObjectPackage, FName(STR("Object")), 1000);
                }
                else
                {
                    Output::send<LogLevel::Warning>(STR("Could not find /Script/CoreUObject package\n"));
                }
            });
        }
        ImGui::SameLine();
        if (ImGui::Button("Find Class in /Script/CoreUObject x1000"))
        {
            TRY([&RunStaticFindObjectWithOuterComparison] {
                UObject* CoreUObjectPackage = FUObjectHashTables::StaticFindObject(nullptr, nullptr, FName(STR("/Script/CoreUObject")), false, true);
                if (CoreUObjectPackage)
                {
                    RunStaticFindObjectWithOuterComparison(STR("Class in /Script/CoreUObject"),
                                                            nullptr, CoreUObjectPackage, FName(STR("Class")), 1000);
                }
                else
                {
                    Output::send<LogLevel::Warning>(STR("Could not find /Script/CoreUObject package\n"));
                }
            });
        }

        // Run all StaticFindObject comparisons
        if (ImGui::Button("Run All StaticFindObject Comparisons"))
        {
            TRY([&RunStaticFindObjectWithOuterComparison] {
                Output::send(STR("========== STATICFINDOBJECT COMPARISON ==========\n\n"));

                // Top-level packages
                Output::send(STR("--- Top-level packages ---\n"));
                RunStaticFindObjectWithOuterComparison(STR("Package /Script/Engine"), nullptr, nullptr, FName(STR("/Script/Engine")), 1000);
                RunStaticFindObjectWithOuterComparison(STR("Package /Script/CoreUObject"), nullptr, nullptr, FName(STR("/Script/CoreUObject")), 1000);

                // Objects with outer
                Output::send(STR("--- Objects with outer ---\n"));
                UObject* EnginePackage = FUObjectHashTables::StaticFindObject(nullptr, nullptr, FName(STR("/Script/Engine")), false, true);
                UObject* CoreUObjectPackage = FUObjectHashTables::StaticFindObject(nullptr, nullptr, FName(STR("/Script/CoreUObject")), false, true);

                if (EnginePackage)
                {
                    RunStaticFindObjectWithOuterComparison(STR("PlayerController in /Script/Engine"), nullptr, EnginePackage, FName(STR("PlayerController")), 1000);
                    RunStaticFindObjectWithOuterComparison(STR("Actor in /Script/Engine"), nullptr, EnginePackage, FName(STR("Actor")), 1000);
                    RunStaticFindObjectWithOuterComparison(STR("Pawn in /Script/Engine"), nullptr, EnginePackage, FName(STR("Pawn")), 1000);
                    RunStaticFindObjectWithOuterComparison(STR("Character in /Script/Engine"), nullptr, EnginePackage, FName(STR("Character")), 1000);
                }

                if (CoreUObjectPackage)
                {
                    RunStaticFindObjectWithOuterComparison(STR("Object in /Script/CoreUObject"), nullptr, CoreUObjectPackage, FName(STR("Object")), 1000);
                    RunStaticFindObjectWithOuterComparison(STR("Class in /Script/CoreUObject"), nullptr, CoreUObjectPackage, FName(STR("Class")), 1000);
                }

                Output::send(STR("========== END STATICFINDOBJECT COMPARISON ==========\n"));
            });
        }

        // Custom path input test (uses StaticFindObjectByPath with path parsing)
        ImGui::Spacing();
        ImGui::Text("Custom Path Test (with path parsing):");
        ImGui::Text("Format: /Package.Object or /Package.Object:SubObject");

        static char customPathInput[512] = "/Script/Engine.PlayerController";
        static int customIterations = 1000;
        ImGui::InputText("Object Path", customPathInput, sizeof(customPathInput));
        ImGui::InputInt("Iterations", &customIterations);
        if (customIterations < 1) customIterations = 1;
        if (customIterations > 100000) customIterations = 100000;

        if (ImGui::Button("Test Custom Path"))
        {
            TRY([&] {
                // Convert to wide string
                std::wstring widePath;
                for (size_t i = 0; customPathInput[i] != '\0'; ++i)
                {
                    widePath += static_cast<wchar_t>(customPathInput[i]);
                }

                Output::send(STR("=== Custom Path Test: {} ({} iterations) ===\n"), widePath, customIterations);

                // Hash table method (with path parsing)
                auto hashStart = std::chrono::high_resolution_clock::now();
                UObject* hashResult = nullptr;
                for (int i = 0; i < customIterations; ++i)
                {
                    hashResult = FUObjectHashTables::StaticFindObjectByPath(nullptr, widePath.c_str(), false, true);
                }
                auto hashEnd = std::chrono::high_resolution_clock::now();
                auto hashTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(hashEnd - hashStart).count();

                // Slow method (with path parsing)
                auto slowStart = std::chrono::high_resolution_clock::now();
                UObject* slowResult = nullptr;
                for (int i = 0; i < customIterations; ++i)
                {
                    slowResult = FUObjectHashTables::StaticFindObjectByPath(nullptr, widePath.c_str(), false, false);
                }
                auto slowEnd = std::chrono::high_resolution_clock::now();
                auto slowTimeUs = std::chrono::duration_cast<std::chrono::microseconds>(slowEnd - slowStart).count();

                Output::send(STR("HashTable: {} us total, {:.2f} us/iter, found: {}\n"),
                             hashTimeUs, static_cast<double>(hashTimeUs) / customIterations,
                             hashResult ? hashResult->GetFullName() : STR("nullptr"));
                Output::send(STR("Slow:      {} us total, {:.2f} us/iter, found: {}\n"),
                             slowTimeUs, static_cast<double>(slowTimeUs) / customIterations,
                             slowResult ? slowResult->GetFullName() : STR("nullptr"));

                if (hashTimeUs > 0 && slowTimeUs > 0)
                {
                    Output::send(STR("HashTable is {:.1f}x faster than Slow\n"),
                                 static_cast<double>(slowTimeUs) / hashTimeUs);
                }

                // Verify results match
                if (hashResult != slowResult)
                {
                    Output::send<LogLevel::Warning>(STR("WARNING: Results don't match! HashTable={}, Slow={}\n"),
                                                     hashResult ? hashResult->GetFullName() : STR("nullptr"),
                                                     slowResult ? slowResult->GetFullName() : STR("nullptr"));
                }
                Output::send(STR("\n"));
            });
        }

        ImGui::SameLine();
        if (ImGui::Button("Quick Test (Just Find)"))
        {
            TRY([&] {
                std::wstring widePath;
                for (size_t i = 0; customPathInput[i] != '\0'; ++i)
                {
                    widePath += static_cast<wchar_t>(customPathInput[i]);
                }

                UObject* result = FUObjectHashTables::StaticFindObjectByPath(nullptr, widePath.c_str(), false, true);
                if (result)
                {
                    Output::send(STR("Found: {}\n"), result->GetFullName());
                }
                else
                {
                    Output::send<LogLevel::Warning>(STR("Object not found: {}\n"), widePath);
                }
            });
        }
    }
} // namespace RC::GUI::Profilers

#endif // UE4SS_PROFILER_TAB