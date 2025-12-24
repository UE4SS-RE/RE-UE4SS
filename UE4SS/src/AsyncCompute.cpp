#include <AsyncCompute.hpp>
#include <Mod/LuaMod.hpp>

#include <cmath>
#include <UE4SSProgram.hpp>
#include <DynamicOutput/DynamicOutput.hpp>

#include <LuaMadeSimple/LuaMadeSimple.hpp>

namespace RC
{
    // Singleton instance
    static std::unique_ptr<AsyncComputeManager> s_instance;

    AsyncComputeManager::AsyncComputeManager() = default;

    AsyncComputeManager::~AsyncComputeManager()
    {
        shutdown();
    }

    auto AsyncComputeManager::get() -> AsyncComputeManager&
    {
        if (!s_instance)
        {
            s_instance = std::make_unique<AsyncComputeManager>();
        }
        return *s_instance;
    }

    auto AsyncComputeManager::register_task_handler(const std::string& task_name, AsyncTaskHandler handler) -> bool
    {
        std::lock_guard<std::mutex> lock(m_handlers_mutex);

        if (m_task_handlers.find(task_name) != m_task_handlers.end())
        {
            return false;  // Already registered
        }

        m_task_handlers[task_name] = std::move(handler);
        return true;
    }

    auto AsyncComputeManager::unregister_task_handler(const std::string& task_name) -> bool
    {
        std::lock_guard<std::mutex> lock(m_handlers_mutex);
        return m_task_handlers.erase(task_name) > 0;
    }

    auto AsyncComputeManager::has_task_handler(const std::string& task_name) const -> bool
    {
        std::lock_guard<std::mutex> lock(m_handlers_mutex);
        return m_task_handlers.find(task_name) != m_task_handlers.end();
    }

    auto AsyncComputeManager::submit_task(
        LuaMod* mod,
        const std::string& task_name,
        LuaType::LuaValue input,
        int32_t callback_ref,
        int32_t callback_thread_ref
    ) -> uint64_t
    {
        if (m_shutdown.load())
        {
            return 0;
        }

        AsyncTaskHandler handler;
        {
            std::lock_guard<std::mutex> lock(m_handlers_mutex);
            auto it = m_task_handlers.find(task_name);
            if (it == m_task_handlers.end())
            {
                return 0;  // Task handler not found
            }
            handler = it->second;  // Copy the handler
        }

        uint64_t handle = m_next_handle.fetch_add(1);

        // Launch async task
        auto future = std::async(std::launch::async, [this, mod, handler = std::move(handler), input = std::move(input), callback_ref, callback_thread_ref, handle]() {
            // Check if cancelled before starting
            {
                std::lock_guard<std::mutex> lock(m_cancelled_mutex);
                if (m_cancelled_tasks.count(handle) > 0)
                {
                    m_cancelled_tasks.erase(handle);
                    return;
                }
            }

            AsyncComputeResult result;
            try
            {
                result = handler(input);
            }
            catch (const std::exception& e)
            {
                result = AsyncComputeResult::Error(std::string("Exception in async task: ") + e.what());
            }
            catch (...)
            {
                result = AsyncComputeResult::Error("Unknown exception in async task");
            }

            // Check if cancelled after completion (don't queue callback)
            {
                std::lock_guard<std::mutex> lock(m_cancelled_mutex);
                if (m_cancelled_tasks.count(handle) > 0)
                {
                    m_cancelled_tasks.erase(handle);
                    return;
                }
            }

            // Queue the completed callback
            {
                std::lock_guard<std::mutex> lock(m_callbacks_mutex);
                m_completed_callbacks.push(PendingCallback{
                    mod,
                    callback_ref,
                    callback_thread_ref,
                    std::move(result)
                });
            }

            // Notify the main event loop that there's work to do
            // This triggers process_completed_tasks to be called
            if (auto* program = &UE4SSProgram::get_program())
            {
                program->queue_event([this]() {
                    process_completed_tasks(1);
                });
            }
        });

        // Store the future to keep the task alive
        {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            // Clean up completed futures first
            m_active_tasks.erase(
                std::remove_if(m_active_tasks.begin(), m_active_tasks.end(), [](std::future<void>& f) {
                    return f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                }),
                m_active_tasks.end()
            );
            m_active_tasks.push_back(std::move(future));
        }

        return handle;
    }

    auto AsyncComputeManager::process_completed_tasks(int max_callbacks) -> void
    {
        for (int i = 0; i < max_callbacks; ++i)
        {
            PendingCallback callback;

            // Pop one callback from the queue
            {
                std::lock_guard<std::mutex> lock(m_callbacks_mutex);
                if (m_completed_callbacks.empty())
                {
                    break;
                }
                callback = std::move(m_completed_callbacks.front());
                m_completed_callbacks.pop();
            }

            // Invoke the Lua callback on the main thread
            try
            {
                auto* lua = callback.mod->main_lua();
                if (!lua)
                {
                    Output::send<LogLevel::Warning>(STR("[AsyncCompute] Mod's Lua state is null, cannot invoke callback\n"));
                    continue;
                }

                lua_State* L = lua->get_lua_state();

                // Get the callback function from registry
                lua->registry().get_function_ref(callback.callback_ref);

                // Create result table: { success = bool, error = string?, result = value? }
                lua_newtable(L);

                lua_pushboolean(L, callback.result.success ? 1 : 0);
                lua_setfield(L, -2, "success");

                if (!callback.result.success)
                {
                    lua_pushstring(L, callback.result.error_message.c_str());
                    lua_setfield(L, -2, "error");
                }

                if (callback.result.success || !callback.result.result.is_nil())
                {
                    LuaType::push_lua_value(L, callback.result.result);
                    lua_setfield(L, -2, "result");
                }

                // Call the callback with the result table
                lua->call_function(1, 0);

                // Unref the callback function now that we're done
                luaL_unref(L, LUA_REGISTRYINDEX, callback.callback_ref);
            }
            catch (const std::exception& e)
            {
                Output::send<LogLevel::Warning>(STR("[AsyncCompute] Error invoking callback: {}\n"), ensure_str(e.what()));
            }
        }
    }

    auto AsyncComputeManager::cancel_task(uint64_t handle) -> bool
    {
        std::lock_guard<std::mutex> lock(m_cancelled_mutex);
        // Mark as cancelled - the task will check this flag and skip queuing the callback
        m_cancelled_tasks.insert(handle);
        return true;
    }

    auto AsyncComputeManager::shutdown() -> void
    {
        m_shutdown.store(true);

        // Wait for all active tasks to complete
        std::vector<std::future<void>> tasks;
        {
            std::lock_guard<std::mutex> lock(m_tasks_mutex);
            tasks = std::move(m_active_tasks);
        }

        for (auto& task : tasks)
        {
            if (task.valid())
            {
                task.wait();
            }
        }

        // Clear pending callbacks
        {
            std::lock_guard<std::mutex> lock(m_callbacks_mutex);
            while (!m_completed_callbacks.empty())
            {
                m_completed_callbacks.pop();
            }
        }
    }

    // Built-in task handlers

    auto register_builtin_async_tasks() -> void
    {
        auto& manager = AsyncComputeManager::get();

        // "sleep" - Simple delay task (mainly for testing)
        manager.register_task_handler("sleep", [](const LuaType::LuaValue& input) -> AsyncComputeResult {
            int64_t ms = 1000;  // Default 1 second

            if (input.is_table())
            {
                for (const auto& [key, value] : input.as_table())
                {
                    if (key.is_string() && key.as_string() == "ms" && value.is_integer())
                    {
                        ms = value.as_integer();
                    }
                }
            }
            else if (input.is_integer())
            {
                ms = input.as_integer();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(ms));

            // Return the elapsed time
            LuaType::LuaValue result;
            std::vector<std::pair<LuaType::LuaValue, LuaType::LuaValue>> result_table;
            result_table.emplace_back(LuaType::LuaValue{std::string("slept_ms")}, LuaType::LuaValue{ms});
            return AsyncComputeResult::Success(LuaType::LuaValue{std::move(result_table)});
        });

        // "compute" - Pure computation (for testing serialization)
        // Input: { iterations = number }
        // Output: { result = number }
        manager.register_task_handler("compute", [](const LuaType::LuaValue& input) -> AsyncComputeResult {
            int64_t iterations = 1000000;

            if (input.is_table())
            {
                for (const auto& [key, value] : input.as_table())
                {
                    if (key.is_string() && key.as_string() == "iterations" && value.is_integer())
                    {
                        iterations = value.as_integer();
                    }
                }
            }

            // Do some computation
            double sum = 0.0;
            for (int64_t i = 0; i < iterations; ++i)
            {
                sum += std::sqrt(static_cast<double>(i));
            }

            std::vector<std::pair<LuaType::LuaValue, LuaType::LuaValue>> result_table;
            result_table.emplace_back(LuaType::LuaValue{std::string("sum")}, LuaType::LuaValue{sum});
            result_table.emplace_back(LuaType::LuaValue{std::string("iterations")}, LuaType::LuaValue{iterations});
            return AsyncComputeResult::Success(LuaType::LuaValue{std::move(result_table)});
        });
    }

} // namespace RC
