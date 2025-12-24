#pragma once

#include <atomic>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <Common.hpp>
#include <LuaType/LuaValue.hpp>

struct lua_State;

namespace RC
{
    struct AsyncComputeResult;
    class LuaMod;

    /**
     * @brief AsyncCompute provides a safe way to run computation off the main thread
     *        and receive results back on the main thread.
     *
     * Key design principles:
     * - Worker code runs on a thread pool, isolated from Lua state
     * - Results are serialized and queued back to the main thread
     * - Callbacks run on the main thread where it's safe to access Lua state
     *
     * Supports two modes:
     * - C++ task handlers: Tasks registered by name, implemented in C++
     * - Lua workers: Pure Lua code runs in isolated Lua state
     */

    /**
     * @brief An isolated Lua state for running pure Lua computations on worker threads.
     *
     * This creates a completely independent Lua state (not a coroutine/thread of the main state)
     * that has no access to UE4SS bindings or the main state's globals.
     *
     * Only safe, standard Lua libraries are loaded:
     * - math, string, table, utf8
     * - io, os (mod authors already have system access)
     * - coroutine (within worker only)
     * - Basic functions: tonumber, tostring, type, pairs, ipairs, next, etc.
     *
     * Excluded for isolation:
     * - debug.* (could break isolation)
     * - load, loadfile, dofile (dynamic code loading)
     * - require, package.* (module system)
     * - All UE4SS bindings
     */
    class RC_UE4SS_API IsolatedLuaState
    {
    public:
        IsolatedLuaState();
        ~IsolatedLuaState();

        // Non-copyable, but movable
        IsolatedLuaState(const IsolatedLuaState&) = delete;
        IsolatedLuaState& operator=(const IsolatedLuaState&) = delete;
        IsolatedLuaState(IsolatedLuaState&& other) noexcept;
        IsolatedLuaState& operator=(IsolatedLuaState&& other) noexcept;

        /**
         * @brief Get the raw Lua state pointer
         */
        auto get() const -> lua_State* { return m_state; }

        /**
         * @brief Check if the state is valid
         */
        auto valid() const -> bool { return m_state != nullptr; }

        /**
         * @brief Load and execute Lua source code that returns a function
         *
         * The source should be something like:
         *   "return function(input) ... return result end"
         *
         * @param source Lua source code
         * @return Error message if failed, empty string on success
         */
        auto load_function(const std::string& source) -> std::string;

        /**
         * @brief Execute the loaded function with the given input
         *
         * @param input The input value to pass to the function
         * @return The result of the computation
         */
        auto execute(const LuaType::LuaValue& input) -> AsyncComputeResult;

        /**
         * @brief Add json.encode and json.decode to this state
         */
        auto add_json_library() -> void;

    private:
        lua_State* m_state{nullptr};

        auto setup_safe_environment() -> void;
        auto remove_unsafe_functions() -> void;
    };

    /**
     * @brief Result of an async computation
     */
    struct AsyncComputeResult
    {
        bool success{true};
        std::string error_message;
        LuaType::LuaValue result;

        static auto Success(LuaType::LuaValue value) -> AsyncComputeResult
        {
            return AsyncComputeResult{true, "", std::move(value)};
        }

        static auto Error(std::string message) -> AsyncComputeResult
        {
            return AsyncComputeResult{false, std::move(message), LuaType::LuaValue{}};
        }
    };

    /**
     * @brief Function signature for C++ async task handlers
     *
     * The handler receives the input LuaValue and should return an AsyncComputeResult.
     * This function runs on a worker thread - do NOT access Lua state or UObjects.
     */
    using AsyncTaskHandler = std::function<AsyncComputeResult(const LuaType::LuaValue& input)>;

    /**
     * @brief Manages async computation tasks
     */
    class RC_UE4SS_API AsyncComputeManager
    {
    public:
        AsyncComputeManager();
        ~AsyncComputeManager();

        // Non-copyable, non-movable
        AsyncComputeManager(const AsyncComputeManager&) = delete;
        AsyncComputeManager& operator=(const AsyncComputeManager&) = delete;
        AsyncComputeManager(AsyncComputeManager&&) = delete;
        AsyncComputeManager& operator=(AsyncComputeManager&&) = delete;

        /**
         * @brief Register a C++ task handler by name
         *
         * @param task_name Unique name for this task type
         * @param handler The handler function (runs on worker thread)
         * @return true if registered successfully, false if name already exists
         */
        auto register_task_handler(const std::string& task_name, AsyncTaskHandler handler) -> bool;

        /**
         * @brief Unregister a task handler
         */
        auto unregister_task_handler(const std::string& task_name) -> bool;

        /**
         * @brief Check if a task handler is registered
         */
        auto has_task_handler(const std::string& task_name) const -> bool;

        /**
         * @brief Submit an async computation task
         *
         * @param mod The mod submitting the task (for callback context)
         * @param task_name Name of the registered task handler
         * @param input Input data (serialized from Lua)
         * @param callback_ref Lua registry reference for the callback function
         * @param callback_thread_ref Lua registry reference for the callback thread
         * @return Handle that can be used to check status (0 if task_name not found)
         */
        auto submit_task(
            LuaMod* mod,
            const std::string& task_name,
            LuaType::LuaValue input,
            int32_t callback_ref,
            int32_t callback_thread_ref
        ) -> uint64_t;

        /**
         * @brief Process completed tasks and invoke callbacks on main thread
         *
         * This should be called from the main thread (e.g., in the event loop).
         * It will invoke Lua callbacks for any completed async tasks.
         *
         * @param max_callbacks Maximum number of callbacks to process per call
         */
        auto process_completed_tasks(int max_callbacks = 10) -> void;

        /**
         * @brief Cancel a pending task by handle
         *
         * Note: If the task is already running, it cannot be cancelled.
         *
         * @return true if task was cancelled, false if not found or already running
         */
        auto cancel_task(uint64_t handle) -> bool;

        /**
         * @brief Shutdown the manager, cancelling all pending tasks
         */
        auto shutdown() -> void;

        /**
         * @brief Get the singleton instance
         */
        static auto get() -> AsyncComputeManager&;

    private:
        struct PendingCallback
        {
            LuaMod* mod;
            int32_t callback_ref;
            int32_t callback_thread_ref;
            AsyncComputeResult result;
        };

        std::unordered_map<std::string, AsyncTaskHandler> m_task_handlers;
        mutable std::mutex m_handlers_mutex;

        std::queue<PendingCallback> m_completed_callbacks;
        std::mutex m_callbacks_mutex;

        std::vector<std::future<void>> m_active_tasks;
        std::mutex m_tasks_mutex;

        std::atomic<uint64_t> m_next_handle{1};
        std::atomic<bool> m_shutdown{false};

        // Track cancelled tasks
        std::unordered_set<uint64_t> m_cancelled_tasks;
        std::mutex m_cancelled_mutex;
    };

    /**
     * @brief Helper to register built-in async task handlers
     */
    auto register_builtin_async_tasks() -> void;

} // namespace RC
