#include <ExternalLuaRepl.hpp>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <fstream>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

#include <DynamicOutput/DynamicOutput.hpp>
#include <Helpers/String.hpp>
#include <LuaLibrary.hpp>
#include <LuaMadeSimple/LuaMadeSimple.hpp>
#include <LuaMadeSimple/LuaObject.hpp>
#include <LuaType/LuaFName.hpp>
#include <LuaType/LuaUEnum.hpp>
#include <LuaType/LuaUObject.hpp>
#include <LuaType/LuaUScriptStruct.hpp>
#include <LuaType/LuaUWorld.hpp>
#include <Mod/LuaMod.hpp>
#include <UE4SSProgram.hpp>
#include <UE4SSRuntime.hpp>
#include <Unreal/Hooks.hpp>
#include <Unreal/UObject.hpp>

#include <fmt/core.h>
#include <glaze/glaze.hpp>
#include <lstate.h>
#include <lua.hpp>

namespace RC
{
    // Protocol 1, described in docs/feature-overview/external-lua-repl.md. A change to either message is a new protocol.
    static constexpr int s_protocol = 1;

    static constexpr auto s_poll_interval = std::chrono::milliseconds{25};
    static constexpr std::uintmax_t s_max_request_size = 64 * 1024;
    static constexpr size_t s_max_queued_requests = 8;
    static constexpr int64_t s_default_timeout_ms = 250;
    static constexpr int64_t s_max_timeout_ms = 5000;
    static constexpr int s_instructions_between_time_checks = 1000;
    static constexpr size_t s_max_values = 64;
    static constexpr size_t s_max_value_length = 4 * 1024;
    static constexpr size_t s_max_print_bytes = 64 * 1024;
    static constexpr size_t s_max_prints = 10000;
    static constexpr size_t s_max_error_length = 16 * 1024;
    static constexpr int s_max_table_entries_counted = 100;
    static constexpr int s_max_answer_write_attempts = 40;
    static constexpr const char* s_chunk_name = "=ExternalLuaRepl";

    struct ExternalLuaReplRequestJson
    {
        int protocol{};
        std::string op{};
        std::string mod{};
        std::string code{};
        std::optional<int64_t> timeout_ms{};
    };

    struct ExternalLuaReplAnswerJson
    {
        int protocol{s_protocol};
        std::string id{};
        bool ok{};
        std::string error{};
        std::vector<std::string> values{};
        std::vector<std::string> prints{};
        std::vector<std::string> mods{};
        bool truncated{};
        double elapsed_ms{};
    };

    // Unknown keys are ignored so that a client can send optional fields to an older UE4SS; the protocol number is
    // what has to match.
    static constexpr glz::opts s_request_read_opts{.error_on_unknown_keys = false};
    // Lua strings may contain any byte. Control characters are escaped here; invalid UTF-8 is replaced before writing.
    struct ExternalLuaReplWriteOpts : glz::opts
    {
        bool escape_control_characters = true;
    };

    namespace
    {
        using Request = ExternalLuaReplRequestJson;
        using Answer = ExternalLuaReplAnswerJson;

        template <typename Function>
        class ScopeExit
        {
          public:
            explicit ScopeExit(Function function) : m_function(std::move(function))
            {
            }
            ~ScopeExit()
            {
                m_function();
            }
            ScopeExit(const ScopeExit&) = delete;
            auto operator=(const ScopeExit&) -> ScopeExit& = delete;

          private:
            Function m_function;
        };

        struct QueuedRequest
        {
            std::string id{};
            std::string mod{};
            std::string code{};
            std::chrono::milliseconds timeout{};
        };

        struct PendingAnswer
        {
            Answer answer{};
            int attempts{};
        };

        // A started mod that requests can target. thread is a Lua thread of the mod's state that only requests run on;
        // it is anchored in that state's registry (LuaMod::m_repl_lua), so it lives until the state is closed.
        struct Target
        {
            ModId mod_id{};
            std::string name{};
            lua_State* thread{};
        };

        // Guards s_targets only and is never held while anything else is locked or any Lua runs.
        // An entry's thread stays valid while LuaMod::game_thread_execution_mutex is held: uninstall removes the entry
        // under that lock before it closes the state.
        std::mutex s_targets_mutex{};
        std::vector<Target> s_targets{};

        struct State
        {
            std::filesystem::path folder{};
            // Guards inbox and answers only; never held while anything else is locked or any Lua runs.
            std::mutex mutex{};
            std::condition_variable_any wake{};
            std::deque<QueuedRequest> inbox{};
            std::vector<PendingAnswer> answers{};
            // Lets the tick callback skip the mutex while there is nothing to do.
            std::atomic<bool> has_requests{};
            std::atomic<bool> is_admitting{};
            Unreal::Hook::GlobalCallbackId tick_callback_id{Unreal::Hook::ERROR_ID};
            std::jthread mailbox{};
        };
        // Never destroyed: stop() ends the mailbox thread, and nothing here may run during static destruction at exit.
        State& s_state = *new State{};

        // Time limit of the request that is running, read by the count hook. The hook is copied into every coroutine
        // the chunk creates, and such a coroutine can be resumed later from any thread, so it is atomic and the hook
        // checks the thread as well.
        std::atomic<bool> s_time_limit_active{};
        std::atomic<std::thread::id> s_time_limit_thread{};
        std::atomic<std::chrono::steady_clock::rep> s_time_limit_deadline{};
        std::atomic<int64_t> s_time_limit_ms{};
        // The thread the request runs on; only read by the hook while s_time_limit_active is set on this OS thread.
        std::atomic<lua_State*> s_time_limit_root{};

        auto make_error_answer(std::string id, std::string error) -> Answer
        {
            Answer answer{};
            answer.id = std::move(id);
            answer.error = std::move(error);
            return answer;
        }

        auto post_answer(Answer answer) -> void
        {
            {
                std::lock_guard<std::mutex> lock{s_state.mutex};
                s_state.answers.push_back(PendingAnswer{std::move(answer)});
            }
            s_state.wake.notify_one();
        }

        // Replaces every byte that is not part of a valid UTF-8 sequence with U+FFFD, so the answer is valid JSON.
        auto to_valid_utf8(std::string_view text) -> std::string
        {
            std::string result{};
            result.reserve(text.size());
            size_t i = 0;
            while (i < text.size())
            {
                const auto lead = static_cast<unsigned char>(text[i]);
                size_t length = lead < 0x80 ? 1 : (lead >> 5) == 0x6 ? 2 : (lead >> 4) == 0xE ? 3 : (lead >> 3) == 0x1E ? 4 : 0;
                bool is_valid = length != 0 && i + length <= text.size();
                uint32_t code_point = length == 1 ? lead : length == 2 ? lead & 0x1F : length == 3 ? lead & 0x0F : lead & 0x07;
                for (size_t k = 1; is_valid && k < length; ++k)
                {
                    const auto continuation = static_cast<unsigned char>(text[i + k]);
                    is_valid = (continuation >> 6) == 0x2;
                    code_point = (code_point << 6) | (continuation & 0x3F);
                }
                if (is_valid && length > 1)
                {
                    static constexpr uint32_t smallest_code_point[]{0, 0, 0x80, 0x800, 0x10000};
                    is_valid = code_point >= smallest_code_point[length] && code_point <= 0x10FFFF && (code_point < 0xD800 || code_point > 0xDFFF);
                }
                if (is_valid)
                {
                    result.append(text.substr(i, length));
                    i += length;
                }
                else
                {
                    result.append("\xEF\xBF\xBD");
                    ++i;
                }
            }
            return result;
        }

        auto clip(std::string text, size_t max_length, bool& truncated) -> std::string
        {
            if (text.size() > max_length)
            {
                text.resize(max_length);
                text.append("...");
                truncated = true;
            }
            return text;
        }

        auto write_answer_file(const Answer& answer) -> bool
        {
            Answer sanitized = answer;
            sanitized.error = to_valid_utf8(answer.error);
            for (auto* strings : {&sanitized.values, &sanitized.prints, &sanitized.mods})
            {
                for (auto& string : *strings)
                {
                    string = to_valid_utf8(string);
                }
            }
            const auto json = glz::write<ExternalLuaReplWriteOpts{}>(sanitized);
            if (!json.has_value())
            {
                return false;
            }

            // Written next to the answer and renamed over it, so a tool never reads a partial answer. The rename
            // replaces an answer of the same name that a tool left behind.
            const auto temporary_path = s_state.folder / (answer.id + ".response.json.tmp");
            const auto answer_path = s_state.folder / (answer.id + ".response.json");
            std::error_code error{};
            // Whatever is at the temporary name is removed first, so the answer goes into a new file in the folder;
            // remove() deletes a link itself, not its target.
            std::filesystem::remove(temporary_path, error);
            {
                std::ofstream file{temporary_path, std::ios::binary | std::ios::trunc};
                file.write(json->data(), static_cast<std::streamsize>(json->size()));
                file.close();
                if (!file)
                {
                    return false;
                }
            }
            std::filesystem::rename(temporary_path, answer_path, error);
            if (error)
            {
                std::filesystem::remove(temporary_path, error);
                return false;
            }
            return true;
        }

        // Writes the new answers and those that failed before. Failed ones stay in 'retry', which only the mailbox
        // thread uses, so they are tried again on later passes, one poll interval apart, rather than at once.
        auto write_answers(std::vector<PendingAnswer>& retry) -> void
        {
            std::vector<PendingAnswer> pending = std::move(retry);
            retry.clear();
            {
                std::lock_guard<std::mutex> lock{s_state.mutex};
                pending.insert(pending.end(), std::make_move_iterator(s_state.answers.begin()), std::make_move_iterator(s_state.answers.end()));
                s_state.answers.clear();
            }
            for (auto& item : pending)
            {
                if (write_answer_file(item.answer))
                {
                    continue;
                }
                // Typically a tool still has an earlier answer of the same name open.
                if (++item.attempts < s_max_answer_write_attempts)
                {
                    retry.push_back(std::move(item));
                }
                else
                {
                    Output::send<LogLevel::Warning>(STR("[ExternalLuaRepl] Could not write the answer to request {}\n"), ensure_str(item.answer.id));
                }
            }
        }

        // "<id>.request.json" with an id of 8 to 32 lowercase hex digits; anything else in the folder is ignored.
        auto request_id_from_file_name(const std::filesystem::path& file_name) -> std::optional<std::string>
        {
            static constexpr std::string_view suffix{".request.json"};
            const auto& name = file_name.native();
            if (name.size() < 8 + suffix.size() || name.size() > 32 + suffix.size())
            {
                return std::nullopt;
            }
            const size_t id_length = name.size() - suffix.size();
            for (size_t i = 0; i < suffix.size(); ++i)
            {
                if (name[id_length + i] != static_cast<std::filesystem::path::value_type>(suffix[i]))
                {
                    return std::nullopt;
                }
            }
            std::string id{};
            for (size_t i = 0; i < id_length; ++i)
            {
                const auto c = name[i];
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')))
                {
                    return std::nullopt;
                }
                id.push_back(static_cast<char>(c));
            }
            return id;
        }

        auto list_targets() -> std::vector<std::string>
        {
            std::lock_guard<std::mutex> lock{s_targets_mutex};
            std::vector<std::string> names{};
            for (const auto& target : s_targets)
            {
                names.push_back(target.name);
            }
            return names;
        }

        auto handle_request(std::string id, const std::string& content, bool is_too_large) -> void
        {
            if (is_too_large)
            {
                post_answer(make_error_answer(std::move(id), fmt::format("The request is larger than {} bytes", s_max_request_size)));
                return;
            }

            Request request{};
            if (const auto error = glz::read<s_request_read_opts>(request, content); error)
            {
                post_answer(make_error_answer(std::move(id), fmt::format("The request is not valid JSON: {}", glz::format_error(error, content))));
                return;
            }
            if (request.protocol != s_protocol)
            {
                auto error = fmt::format("Unsupported protocol {}; this UE4SS speaks protocol {}", request.protocol, s_protocol);
                post_answer(make_error_answer(std::move(id), std::move(error)));
                return;
            }

            if (request.op == "list")
            {
                Answer answer{};
                answer.id = std::move(id);
                answer.ok = true;
                answer.mods = list_targets();
                post_answer(std::move(answer));
                return;
            }
            if (request.op != "exec")
            {
                post_answer(make_error_answer(std::move(id), fmt::format("Unknown op '{}'; expected 'exec' or 'list'", request.op)));
                return;
            }
            if (request.mod.empty() || request.code.empty())
            {
                post_answer(make_error_answer(std::move(id), "'exec' needs 'mod' and 'code'"));
                return;
            }
            const auto timeout_ms = request.timeout_ms.value_or(s_default_timeout_ms);
            if (timeout_ms < 1 || timeout_ms > s_max_timeout_ms)
            {
                post_answer(make_error_answer(std::move(id), fmt::format("'timeout_ms' must be between 1 and {}", s_max_timeout_ms)));
                return;
            }

            Output::send<LogLevel::Verbose>(STR("[ExternalLuaRepl] Request {} for mod '{}'\n"), ensure_str(id), ensure_str(request.mod));
            std::string refusal{};
            {
                std::lock_guard<std::mutex> lock{s_state.mutex};
                if (!s_state.is_admitting)
                {
                    refusal = "UE4SS is shutting down";
                }
                else if (s_state.inbox.size() >= s_max_queued_requests)
                {
                    refusal = fmt::format("Busy: {} requests are already waiting", s_state.inbox.size());
                }
                else
                {
                    s_state.inbox.push_back(QueuedRequest{id, std::move(request.mod), std::move(request.code), std::chrono::milliseconds{timeout_ms}});
                    s_state.has_requests = true;
                }
            }
            if (!refusal.empty())
            {
                post_answer(make_error_answer(std::move(id), std::move(refusal)));
            }
        }

        auto read_requests() -> void
        {
            std::vector<std::pair<std::string, std::filesystem::path>> found{};
            std::error_code error{};
            for (std::filesystem::directory_iterator it{s_state.folder, error}, end{}; !error && it != end; it.increment(error))
            {
                auto id = request_id_from_file_name(it->path().filename());
                if (!id)
                {
                    continue;
                }
                // symlink_status: a link or junction is not a regular file here, so it is skipped rather than followed.
                std::error_code status_error{};
                const auto status = it->symlink_status(status_error);
                if (status_error || status.type() != std::filesystem::file_type::regular)
                {
                    continue;
                }
                found.emplace_back(std::move(*id), it->path());
            }

            for (auto& [id, path] : found)
            {
                std::string content(s_max_request_size + 1, '\0');
                {
                    std::ifstream file{path, std::ios::binary};
                    if (!file)
                    {
                        // Typically still open in the tool; try again on the next pass.
                        continue;
                    }
                    file.read(content.data(), static_cast<std::streamsize>(content.size()));
                    content.resize(static_cast<size_t>(file.gcount()));
                }
                // A request runs at most once: only a request whose file could be removed is handled.
                std::error_code remove_error{};
                if (!std::filesystem::remove(path, remove_error) || remove_error)
                {
                    continue;
                }
                const bool is_too_large = content.size() > s_max_request_size;
                handle_request(std::move(id), content, is_too_large);
            }
        }

        auto mailbox_loop(std::stop_token stop_token) -> void
        {
            std::vector<PendingAnswer> retry{};
            while (!stop_token.stop_requested())
            {
                write_answers(retry);
                read_requests();
                std::unique_lock<std::mutex> lock{s_state.mutex};
                s_state.wake.wait_for(lock, stop_token, s_poll_interval, [] {
                    return !s_state.answers.empty();
                });
            }

            // The tick callback is unregistered by now; answer what it will never run.
            {
                std::lock_guard<std::mutex> lock{s_state.mutex};
                for (auto& request : s_state.inbox)
                {
                    s_state.answers.push_back(PendingAnswer{make_error_answer(std::move(request.id), "UE4SS is shutting down")});
                }
                s_state.inbox.clear();
                s_state.has_requests = false;
            }
            write_answers(retry);
        }

        // Count hook installed on the request's thread for the duration of the request.
        auto time_limit_hook(lua_State* L, [[maybe_unused]] lua_Debug* debug) -> void
        {
            if (!s_time_limit_active.load(std::memory_order_acquire) || s_time_limit_thread.load(std::memory_order_relaxed) != std::this_thread::get_id())
            {
                // A coroutine that a request created inherited this hook, and it is being resumed outside that request.
                lua_sethook(L, nullptr, 0, 0);
                return;
            }
            if (std::chrono::steady_clock::now().time_since_epoch().count() >= s_time_limit_deadline.load(std::memory_order_relaxed))
            {
                // From now on raised before every instruction, on this thread and on the request's own thread. With
                // checks 1000 instructions apart, a loop around pcall would catch every one of these errors, because
                // the next check always falls inside the next pcall; checked on every instruction, the error is raised
                // in the loop itself as soon as a pcall returns.
                lua_sethook(L, &time_limit_hook, LUA_MASKCOUNT, 1);
                if (auto* root = s_time_limit_root.load(std::memory_order_relaxed); root && root != L)
                {
                    lua_sethook(root, &time_limit_hook, LUA_MASKCOUNT, 1);
                }

                // The error skips message handlers. Lua calls an enclosing xpcall's handler from inside this hook, where
                // hooks are off (luaD_hook, luaG_errormsg), so a handler that loops could never be stopped. The pcall
                // that catches the error restores errfunc (luaD_pcall, finishpcallk). The traceback is built here instead.
                luaL_checkstack(L, 2, nullptr);
                lua_pushfstring(L, "the request ran longer than its time limit of %d ms", static_cast<int>(s_time_limit_ms.load(std::memory_order_relaxed)));
                luaL_traceback(L, L, lua_tostring(L, -1), 0);
                L->errfunc = 0;
                // luaD_hook turned hooks off for this call and turns them back on when the hook returns, which this one
                // does not. A pcall that catches the error restores them, but a coroutine that it kills keeps them off
                // (lua_resume), so the __close handlers that coroutine.wrap or coroutine.close then run would be
                // unchecked.
                L->allowhook = 1;
                lua_error(L);
            }
        }

        auto error_object_to_string(lua_State* L, int index) -> std::string
        {
            if (lua_type(L, index) == LUA_TSTRING)
            {
                size_t length{};
                const char* text = lua_tolstring(L, index, &length);
                return std::string{text, length};
            }
            return fmt::format("(error object is a {} value)", luaL_typename(L, index));
        }

        auto message_handler(lua_State* L) -> int
        {
            const char* message = lua_type(L, 1) == LUA_TSTRING ? lua_tostring(L, 1) : nullptr;
            if (!message)
            {
                message = lua_pushfstring(L, "(error object is a %s value)", luaL_typename(L, 1));
            }
            luaL_traceback(L, L, message, 1);
            return 1;
        }

        // The UObject that a UE4SS userdata holds, or nullopt if the value is not one of the UE4SS types that hold
        // one. luaL_testudata compares the value's metatable with the one UE4SS registered under that name, so a
        // userdata given a look-alike metatable (debug.setmetatable) is never read as one of them.
        auto uobject_in_userdata(lua_State* L, int index, const std::string& type_name) -> std::optional<Unreal::UObject*>
        {
            // Types derived from UObjectBase, read through LuaType::UObject as UE4SS itself does (call_ufunction_from_lua).
            // LuaType::UStruct registers under "UClass".
            static constexpr std::string_view object_base_types[]{
                    "UObject",
                    "AActor",
                    "UClass",
                    "UFunction",
                    "UInterface",
                    "UDataTable",
            };
            void* userdata = luaL_testudata(L, index, type_name.c_str());
            if (!userdata)
            {
                return std::nullopt;
            }
            if (std::ranges::find(object_base_types, type_name) != std::end(object_base_types))
            {
                return static_cast<LuaType::UObject*>(userdata)->get_remote_cpp_object();
            }
            if (type_name == "UWorld")
            {
                return static_cast<LuaType::UWorld*>(userdata)->get_remote_cpp_object();
            }
            if (type_name == "UEnum")
            {
                return static_cast<LuaType::UEnum*>(userdata)->get_remote_cpp_object();
            }
            if (type_name == "UScriptStruct")
            {
                return static_cast<LuaType::UScriptStruct*>(userdata)->get_local_cpp_object().script_struct;
            }
            return std::nullopt;
        }

        // Display string for one value, without calling metamethods or any other Lua code.
        auto format_value(lua_State* L, int index) -> std::string
        {
            index = lua_absindex(L, index);
            if (!lua_checkstack(L, 3))
            {
                return "(no stack space to format this value)";
            }
            switch (lua_type(L, index))
            {
            case LUA_TNIL:
                return "nil";
            case LUA_TBOOLEAN:
                return lua_toboolean(L, index) ? "true" : "false";
            case LUA_TNUMBER: {
                // Converting a copy formats it exactly like tostring() does.
                lua_pushvalue(L, index);
                size_t length{};
                const char* text = lua_tolstring(L, -1, &length);
                std::string result{text, length};
                lua_pop(L, 1);
                return result;
            }
            case LUA_TSTRING: {
                size_t length{};
                const char* text = lua_tolstring(L, index, &length);
                // One byte more than fits, so that the caller's clip() marks the value as truncated.
                std::string result{"\""};
                result.append(text, std::min(length, s_max_value_length + 1));
                result.append("\"");
                return result;
            }
            case LUA_TTABLE: {
                int count = 0;
                bool counted_all = true;
                lua_pushnil(L);
                while (lua_next(L, index) != 0)
                {
                    lua_pop(L, 1);
                    if (++count >= s_max_table_entries_counted)
                    {
                        lua_pop(L, 1);
                        counted_all = false;
                        break;
                    }
                }
                return fmt::format("table: {} ({}{} entries)", lua_topointer(L, index), count, counted_all ? "" : "+");
            }
            case LUA_TFUNCTION: {
                lua_Debug info{};
                lua_pushvalue(L, index);
                lua_getinfo(L, ">S", &info);
                if (info.what && std::string_view{info.what} == "C")
                {
                    return fmt::format("function: builtin: {}", lua_topointer(L, index));
                }
                return fmt::format("function: {}:{}", info.short_src, info.linedefined);
            }
            case LUA_TUSERDATA: {
                std::string type_name{"userdata"};
                if (lua_getmetatable(L, index))
                {
                    lua_pushliteral(L, "__name");
                    if (lua_rawget(L, -2) == LUA_TSTRING)
                    {
                        type_name = lua_tostring(L, -1);
                    }
                    lua_pop(L, 2);
                }
                if (const auto held_object = uobject_in_userdata(L, index, type_name))
                {
                    // The same validity test as UObject:IsValid().
                    auto* object = *held_object;
                    if (object && object != LuaMadeSimple::Type::special_invalid_ptr() && LuaType::is_object_in_global_unreal_object_map(object) &&
                        !object->IsUnreachable())
                    {
                        return fmt::format("{}: {}", type_name, to_utf8_string(object->GetFullName()));
                    }
                    return fmt::format("{}: (invalid)", type_name);
                }
                // LuaType::FName registers its metatable under this name (LuaFName.cpp), not "FName".
                if (auto* name = static_cast<LuaType::FName*>(luaL_testudata(L, index, "FNameUserdata")))
                {
                    return fmt::format("FName: \"{}\"", to_utf8_string(name->get_local_cpp_object().ToString()));
                }
                return fmt::format("{}: {}", type_name, lua_touserdata(L, index));
            }
            case LUA_TLIGHTUSERDATA:
                return fmt::format("lightuserdata: {}", lua_touserdata(L, index));
            case LUA_TTHREAD:
                return fmt::format("thread: {}", lua_topointer(L, index));
            default:
                return luaL_typename(L, index);
            }
        }

        class AnswerPrintCapture : public LuaLibrary::PrintCapture
        {
          public:
            explicit AnswerPrintCapture(Answer& answer) : m_answer(answer)
            {
            }

            auto on_print(std::string_view text) -> void override
            {
                if (m_bytes >= s_max_print_bytes || m_answer.prints.size() >= s_max_prints)
                {
                    m_answer.truncated = true;
                    return;
                }
                if (text.size() > s_max_print_bytes - m_bytes)
                {
                    text = text.substr(0, s_max_print_bytes - m_bytes);
                    m_answer.truncated = true;
                }
                m_bytes += text.size();
                m_answer.prints.emplace_back(text);
            }

          private:
            Answer& m_answer;
            size_t m_bytes{};
        };

        struct RunContext
        {
            const QueuedRequest& request;
            // Built before entering Lua, so that nothing in run_chunk allocates before the chunk has run.
            std::string expression{};
            Answer& answer;
        };

        // Runs inside lua_pcall on the request's thread, so a Lua error raised in here cannot escape into the engine.
        auto run_chunk(lua_State* L) -> int
        {
            auto& context = *static_cast<RunContext*>(lua_touserdata(L, 1));
            lua_settop(L, 0);
            lua_pushcfunction(L, &message_handler);

            // Like the Lua Debugger's REPL: as an expression first, so that "1 + 1" returns 2, then as statements.
            // Text only: precompiled chunks can corrupt the VM.
            const auto& code = context.request.code;
            int status = luaL_loadbufferx(L, context.expression.data(), context.expression.size(), s_chunk_name, "t");
            if (status == LUA_ERRSYNTAX)
            {
                lua_pop(L, 1);
                status = luaL_loadbufferx(L, code.data(), code.size(), s_chunk_name, "t");
            }
            if (status == LUA_OK)
            {
                status = lua_pcall(L, 0, LUA_MULTRET, 1);
            }

            bool answer_failed = false;
            try
            {
                if (status != LUA_OK)
                {
                    context.answer.error = clip(error_object_to_string(L, -1), s_max_error_length, context.answer.truncated);
                }
                else
                {
                    const int top = lua_gettop(L);
                    for (int i = 2; i <= top; ++i)
                    {
                        if (context.answer.values.size() >= s_max_values)
                        {
                            context.answer.truncated = true;
                            break;
                        }
                        context.answer.values.push_back(clip(format_value(L, i), s_max_value_length, context.answer.truncated));
                    }
                    context.answer.ok = true;
                }
            }
            catch (...)
            {
                answer_failed = true;
            }
            if (answer_failed)
            {
                context.answer.ok = false;
                return luaL_error(L, "could not build the answer");
            }
            return 0;
        }

        auto find_target_thread(std::string_view mod_name) -> lua_State*
        {
            std::lock_guard<std::mutex> lock{s_targets_mutex};
            const auto it = std::ranges::find(s_targets, mod_name, &Target::name);
            return it == s_targets.end() ? nullptr : it->thread;
        }

        auto run_request(const QueuedRequest& request) -> Answer
        {
            Answer answer{};
            answer.id = request.id;

            // Held from the lookup until the request has finished: LuaMod::uninstall takes this lock before it removes
            // the mod from s_targets and closes its state. It is the lock engine_tick_hook holds for its batches.
            std::lock_guard<std::recursive_mutex> execution_guard{LuaMod::game_thread_execution_mutex()};
            if (UE4SSProgram::get_program().are_mods_being_touched())
            {
                answer.error = "Mods are being reinstalled; try again";
                return answer;
            }
            lua_State* thread = find_target_thread(request.mod);
            if (!thread)
            {
                answer.error = fmt::format("No started Lua mod is named '{}'", request.mod);
                return answer;
            }

            RunContext context{request, "return " + request.code, answer};
            AnswerPrintCapture print_capture{answer};
            const auto started_at = std::chrono::steady_clock::now();

            // Like an ExecuteInGameThread action: game-thread actions that the chunk's UFunction calls would otherwise
            // run inside it wait for the next frame instead.
            const bool was_executing_game_action = LuaMod::m_is_currently_executing_game_action;
            LuaMod::m_is_currently_executing_game_action = true;
            auto* const previous_print_capture = LuaLibrary::set_print_capture(&print_capture);
            s_time_limit_ms.store(request.timeout.count(), std::memory_order_relaxed);
            s_time_limit_deadline.store((started_at + request.timeout).time_since_epoch().count(), std::memory_order_relaxed);
            s_time_limit_thread.store(std::this_thread::get_id(), std::memory_order_relaxed);
            s_time_limit_root.store(thread, std::memory_order_relaxed);
            s_time_limit_active.store(true, std::memory_order_release);
            ScopeExit restore{[&] {
                s_time_limit_active.store(false, std::memory_order_release);
                s_time_limit_root.store(nullptr, std::memory_order_relaxed);
                lua_sethook(thread, nullptr, 0, 0);
                lua_settop(thread, 0);
                LuaLibrary::set_print_capture(previous_print_capture);
                LuaMod::m_is_currently_executing_game_action = was_executing_game_action;
            }};

            lua_settop(thread, 0);
            lua_sethook(thread, &time_limit_hook, LUA_MASKCOUNT, s_instructions_between_time_checks);
            lua_pushcfunction(thread, &run_chunk);
            lua_pushlightuserdata(thread, &context);
            if (lua_pcall(thread, 1, 0, 0) != LUA_OK)
            {
                answer.ok = false;
                answer.error = clip(error_object_to_string(thread, -1), s_max_error_length, answer.truncated);
            }
            answer.elapsed_ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started_at).count();
            return answer;
        }

        auto take_request() -> std::optional<QueuedRequest>
        {
            std::lock_guard<std::mutex> lock{s_state.mutex};
            if (s_state.inbox.empty())
            {
                s_state.has_requests = false;
                return std::nullopt;
            }
            auto request = std::move(s_state.inbox.front());
            s_state.inbox.pop_front();
            s_state.has_requests = !s_state.inbox.empty();
            return request;
        }

        auto on_engine_tick([[maybe_unused]] Unreal::Hook::TCallbackIterationData<void>& callback_iteration_data,
                            [[maybe_unused]] Unreal::UEngine* engine,
                            [[maybe_unused]] float delta_seconds,
                            [[maybe_unused]] bool is_idle) -> void
        {
            // Only ever touched on the game thread. Guards against a request that makes the engine tick again.
            static bool s_is_running_request{};
            if (s_is_running_request || !s_state.has_requests.load(std::memory_order_acquire))
            {
                return;
            }
            auto request = take_request();
            if (!request)
            {
                return;
            }
            s_is_running_request = true;
            ScopeExit reset{[] {
                s_is_running_request = false;
            }};
            // Nothing may propagate into the engine's tick.
            try
            {
                Answer answer{};
                try
                {
                    answer = run_request(*request);
                }
                catch (const std::exception& e)
                {
                    answer = make_error_answer(request->id, fmt::format("Internal error: {}", e.what()));
                }
                post_answer(std::move(answer));
            }
            catch (...)
            {
                Output::send<LogLevel::Error>(STR("[ExternalLuaRepl] Could not answer request {}\n"), ensure_str(request->id));
            }
        }
    } // namespace

    auto ExternalLuaRepl::is_enabled() -> bool
    {
        return UE4SSProgram::settings_manager.Debug.ExternalLuaReplEnabled;
    }

    auto ExternalLuaRepl::start(const std::filesystem::path& working_directory) -> void
    {
        if (!is_enabled() || s_state.mailbox.joinable())
        {
            return;
        }
        if (!UE4SSRuntime::IsEngineTickAvailable())
        {
            Output::send<LogLevel::Error>(STR("[ExternalLuaRepl] Not started: requests run from UEngine::Tick, which was not found in this game\n"));
            return;
        }

        const auto folder = working_directory / STR("LuaRepl");
        std::error_code error{};
        std::filesystem::create_directory(folder, error);
        const auto status = std::filesystem::symlink_status(folder, error);
        if (error || status.type() != std::filesystem::file_type::directory)
        {
            Output::send<LogLevel::Error>(STR("[ExternalLuaRepl] Not started: '{}' is not a directory (links and junctions are not accepted)\n"),
                                          ensure_str(folder));
            return;
        }

        s_state.folder = folder;
        s_state.is_admitting = true;
        // The thread first: if it cannot be started, nothing has been registered yet.
        try
        {
            s_state.mailbox = std::jthread{&mailbox_loop};
        }
        catch (const std::system_error& e)
        {
            s_state.is_admitting = false;
            Output::send<LogLevel::Error>(STR("[ExternalLuaRepl] Not started: could not start its thread: {}\n"), ensure_str(e.what()));
            return;
        }
        s_state.tick_callback_id = Unreal::Hook::RegisterEngineTickPreCallback(&on_engine_tick, {false, false, STR("UE4SS"), STR("ExternalLuaRepl")});
        if (s_state.tick_callback_id == Unreal::Hook::ERROR_ID)
        {
            Output::send<LogLevel::Error>(STR("[ExternalLuaRepl] Not started: could not register the UEngine::Tick callback\n"));
            stop();
            return;
        }

        Output::send<LogLevel::Warning>(STR("[ExternalLuaRepl] Enabled. Any program that can write to '{}' can run Lua in this game's Lua mods.\n"),
                                        ensure_str(folder));
    }

    auto ExternalLuaRepl::stop() -> void
    {
        {
            std::lock_guard<std::mutex> lock{s_state.mutex};
            s_state.is_admitting = false;
        }
        // Not under any lock: unregistering waits for a running callback, which takes the execution lock.
        if (s_state.tick_callback_id != Unreal::Hook::ERROR_ID)
        {
            Unreal::Hook::UnregisterCallback(s_state.tick_callback_id);
            s_state.tick_callback_id = Unreal::Hook::ERROR_ID;
        }
        if (s_state.mailbox.joinable())
        {
            s_state.mailbox.request_stop();
            s_state.mailbox.join();
        }
    }

    auto ExternalLuaRepl::add_mod(const LuaMod& mod) -> void
    {
        if (!mod.m_repl_lua)
        {
            return;
        }
        std::lock_guard<std::mutex> lock{s_targets_mutex};
        s_targets.push_back(Target{mod.get_id(), to_utf8_string(mod.get_name()), mod.m_repl_lua->get_lua_state()});
    }

    auto ExternalLuaRepl::remove_mod(const LuaMod& mod) -> void
    {
        std::lock_guard<std::mutex> lock{s_targets_mutex};
        std::erase_if(s_targets, [&](const Target& target) {
            return target.mod_id == mod.get_id();
        });
    }
} // namespace RC
