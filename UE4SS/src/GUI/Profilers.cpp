#include <GUI/Profilers.hpp>

#ifdef UE4SS_PROFILER_TAB

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
        }

        ImGui::Text("Pool Sizes: Actors=%llu, Classes=%llu, ActorClasses=%llu",
                    ObjectSearcherProfiler::FastStats.LastPoolSizeActors.load(std::memory_order_relaxed),
                    ObjectSearcherProfiler::FastStats.LastPoolSizeClasses.load(std::memory_order_relaxed),
                    ObjectSearcherProfiler::FastStats.LastPoolSizeActorClasses.load(std::memory_order_relaxed));

        // Fast path stats
        ImGui::Spacing();
        ImGui::Text("=== Fast Path (Pool) ===");
        const auto& fast = ObjectSearcherProfiler::FastStats;
        ImGui::Text("Add:    %llu calls, avg %.3f us, total %.2f ms",
                    fast.AddCount.load(std::memory_order_relaxed),
                    fast.AvgAddTimeUs(),
                    fast.TotalAddTimeMs());
        ImGui::Text("Remove: %llu calls, avg %.3f us, total %.2f ms",
                    fast.RemoveCount.load(std::memory_order_relaxed),
                    fast.AvgRemoveTimeUs(),
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
    }
} // namespace RC::GUI::Profilers

#endif // UE4SS_PROFILER_TAB