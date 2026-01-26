#pragma once

// EventViewerMod: Centralized UI help strings.
//
// Keeping these in one place makes it easier to tweak copy without touching the main UI code.

// EventViewerMod: Small user-facing help strings shown in the UI.

#include <imgui.h>

#define FILTER_NOTE                                                                                                                                            \
    "Note that filters don't affect the stack depth; they will always be shown as-is to indicate "                                                             \
    "callers that may be potentially filtered out. You can right click any function while paused "                                                             \
    "and select \"Show Call Stack\" to see the unfiltered call stack for that function.\n\n"                                                                   \
    "Additionally, filters are applied to both incoming calls and the current history of all calls "                                                           \
    "in all threads."

namespace RC::EventViewerMod
{
    // From ImGui demo
    inline static void HelpMarker(const char* desc)
    {
        ImGui::SameLine();
        ImGui::TextDisabled("(?)");
        if (ImGui::BeginItemTooltip())
        {
            ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
            ImGui::TextUnformatted(desc);
            ImGui::PopTextWrapPos();
            ImGui::EndTooltip();
        }
    }

    struct HelpStrings
    {
        inline static constexpr auto HELP_TARGET =
                "Filter by which target should be shown.\n\n"
                "Calls are prefixed by the target that called them with their initials.\n\n"
                "A common path that many function calls follow/cause is ProcessEvent->ProcessInternal->ProcessLocalScriptFunction.\n\n" FILTER_NOTE;

        inline static constexpr auto HELP_MODE = "Select the mode that the view should run in.\n\n"
                                                 "Stack: View a filtered stream (see below) of the live call stack for the selected Target.\n\n"
                                                 "Frequency: View a table of all of the functions called so far with the number of times "
                                                 "each function has been called.";

        inline static constexpr auto HELP_LIST_FILTER =
                "Whitelist/Blacklist filters both accept comma-separate lists of strings with case-insensitive matching.\n\n"
                "Note that you can press enter to apply them when focused as well as press the button.\n\n"
                "If you don't have any filters, then all calls that match your Target and \"Show Builtin Tick Functions\" settings "
                "will be shown.\n\n"
                "The caller's name and the function's name make up a call's name, in the form of \"Caller.Function\".\n\n"
                "For a call to pass the whitelist filter, the call must have at least one of the comma-separated strings as a substring of its name. "
                "Only having a whitelist filter is effectively the same as a normal search.\n\n"
                "For a call to pass the blacklist filter, the call must not have any of the comma-separated strings as a substring of its name.\n\n"
                "The whitelist filter applies before the blacklist filter, and each call must pass them in that order to be visible.\n\n" FILTER_NOTE;

        inline static constexpr auto HELP_THREAD = "Select the thread to monitor.\n\n"
                                                   "This generally tries to default to the game thread, marked with \"(Game)\".";

        inline static constexpr auto HELP_CLEAR = "Clears this thread's call stack and frequency counter.";

        inline static constexpr auto HELP_CLEAR_ALL = "Clears all call stacks and frequency counters. This also removes all known threads.";

        inline static constexpr auto HELP_SAVE = "Save the current filtered view to a text file.\n\n"
                                                 "This means that if the Mode is \"Call Stack\", the current call stack view "
                                                 "is saved, with all of the same filters applied, and if the Mode is \"Frequency\", "
                                                 "the table of frequencies is saved.\n\n"
                                                 "The file is saved in Mods\\EventViewerMod\\captures from the ue4ss directory.";

        inline static constexpr auto HELP_SAVE_ALL = "Save all views to a text file.\n\n"
                                                     "The file is saved in Mods\\EventViewerMod\\captures from the ue4ss directory.";

        inline static constexpr auto HELP_SHOW_BUILTIN_TICK = "Attempts to automatically filter out \"Tick\" and \"ReceiveTick\" functions from the view.\n\n"
                                                              "This is inherently faster than blacklisting \"Tick\", but may not block all \"Tick\" functions.";

        inline static constexpr auto HELP_CEMODAL_SHOW_FULL_CONTEXT =
                "Shows all function calls associated with the selected call.\n\n"
                "Note that this whole view may differ from what you see in the normal Call Stack Mode since "
                "this has no filters applied at all.";

        inline static constexpr auto HELP_CEMODAL_SAVE = "Saves this entry's call stack to a text file, respecting the \"Show Full Context\" setting.\n\n"
                                                         "The file is saved in Mods\\EventViewerMod\\captures from the ue4ss directory.";

        inline static constexpr auto HELP_MAX_MS_READ_TIME =
                "The max amount of time the ImGui thread will spend dequeuing calls in milliseconds in a frame.\n\n"
                "Basically, the ImGui thread will dequeue up to \"Max Count Per Iteration\", until the queue is empty, or "
                "until it hits this time limit.";
        inline static constexpr auto HELP_MAX_COUNT_PER_ITERATION =
                "The max amount of calls that will be dequeued in a frame.\n\n"
                "Basically, the ImGui thread will dequeue up to \"Max Count Per Iteration\", until the queue is empty, or "
                "until it hits the time limit in \"Max MS Read Time\".";
        inline static constexpr auto HELP_QUEUE_PROFILE_VALUES =
                "Enqueue Avg (microseconds): The average time it takes a hook to enqueue a call. It should be as low as possible.\n\n"
                "Dequeue Avg (microseconds): The average time it takes for the ImGui thread to dequeue a single call. This should be as low "
                "as possible, though it's likely to increase over time.\n\n"
                "Pending Avg (calls): The average amount of calls that are left in the queue after the ImGui thread finishes an iteration of"
                "dequeuing. Should be 0 or very close to it.\n\n"
                "Time Slot Exceeded Count: Amount of times the ImGui thread exceeded the time set in \"Max MS Read Time\"."
                "Should be 0, or at least rarely moving.";

        inline static constexpr auto HELP_TEXT_VIRTUALIZATION_COUNT = "The total amount of calls that can be streamed into ImGui while running.\n\n"
                                                                      "Changing this value will clear all threads.\n\n"
                                                                      "When scrolling up while paused, you'll eventually run into a \"Load More\" button, "
                                                                      "where the number of calls loaded doubles every time from this number.\n\n"
                                                                      "This doesn't affect filtering, nor does it erase calls, it just controls how much is in "
                                                                      "ImGui at a time (apart from clearing the threads when changing this value).\n\n"
                                                                      "Higher values means making the \"Load More\" button appear later, but can"
                                                                      "come in at a hefty performance cost.";

        inline static constexpr auto HELP_ADD_CALLER_AND_FUNC_NAME_WARNING =
                "WARNING: Adding Caller or Caller + Function name to a filter may block output completely in "
                "Frequency Mode, since the Frequency Mode is only aware of the function name!";
    };
} // namespace RC::EventViewerMod

#undef FILTER_NOTE