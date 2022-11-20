#include <stdio.h>
#include <cstdlib>
#include <cstdint>
#include <bit>

#include <LuaScriptMemoryAccess.hpp>
#include <LuaBindings/States/MainState/Main.hpp>

namespace RC
{
    auto AllocateMemory(size_t Size) -> MemoryItem*
    {
        return static_cast<MemoryItem*>(std::malloc(Size));
    }
    auto FreeMemory(MemoryItem* memory_item) -> void
    {
        std::free(memory_item);
    }
    auto FreeMemory(uintptr_t memory) -> void
    {
        std::free(std::bit_cast<void*>(memory));
    }

    enum class ByteReadWriteMode
    {
        SingleByte,
        MultiByte,
    };

    template<typename NumberType>
    auto read_generic_number_wrapper(lua_State* lua_state, ByteReadWriteMode byte_read_write_mode = ByteReadWriteMode::SingleByte) -> int
    {
        NumberType* data{};

        if (lua_isinteger(lua_state, 1))
        {
            data = std::bit_cast<NumberType*>(lua_tointeger(lua_state, 1));
        }
        else if (lua_islightuserdata(lua_state, 1))
        {
            data = static_cast<NumberType*>(lua_touserdata(lua_state, 1));
        }
        else if (lua_isuserdata(lua_state, 1))
        {
            auto get_address_type = lua_getfield(lua_state, 1, "GetAddress");
            if (get_address_type != LUA_TFUNCTION)
            {
                luaL_error(lua_state, "Userdata is incompatible with memory functions because it has has no 'GetAddress' function.");
            }

            lua_pushvalue(lua_state, 1); // Duplicate the userdata because pcall will pop it from the stack for the first function param (self).

            if (int status = lua_pcall(lua_state, 1, 1, 0); status != LUA_OK)
            {
                luaL_error(lua_state, LuaBindings::resolve_status_message(lua_state, status).c_str());
            }

            if (!lua_isinteger(lua_state, -1) && !lua_isnil(lua_state, -1))
            {
                luaL_error(lua_state, "Userdata is incompatible with memory functions because its 'GetAddress' function returned non-integer.");
            }

            if (lua_isnil(lua_state, -1))
            {
                luaL_error(lua_state, "Cannot place a call to a memory function with a nullptr.");
            }

            data = std::bit_cast<NumberType*>(lua_tointeger(lua_state, -1));
            lua_pop(lua_state, 1);
        }

        //auto memory_item = MemoryItem{data};
        //LuaBindings::lua_Userdata_to_lua_from_heap<"__RC_MemoryItemMetatable", ::RC::MemoryItem, false>(lua_state, memory_item, 0);

        bool return_as_table{};
        if (lua_isboolean(lua_state, 3))
        {
            return_as_table = lua_toboolean(lua_state, 3);
            lua_pop(lua_state, 1);
        }

        if (return_as_table)
        {
            lua_newtable(lua_state);
        }

        if (byte_read_write_mode == ByteReadWriteMode::SingleByte)
        {
            if (return_as_table) { lua_pushinteger(lua_state, 1); }
            lua_pushinteger(lua_state, *data);
            if (return_as_table) { lua_rawset(lua_state, -3); }
            return 1;
        }
        else
        {
            if (!lua_isinteger(lua_state, 2))
            {
                luaL_error(lua_state, "Cannot read bytes in MultiByte mode without specifying how many bytes to read.");
            }

            auto num_bytes_to_read = lua_tointeger(lua_state, 2);
            if (num_bytes_to_read < 1) { luaL_error(lua_state, "Must specify at least one byte to read. Specified: 0 (param #2)"); }

            for (lua_Integer i = 0; i < num_bytes_to_read; ++i)
            {
                if (return_as_table) { lua_pushinteger(lua_state, i + 1); }
                lua_pushinteger(lua_state, *std::bit_cast<NumberType*>(std::bit_cast<uint8_t*>(data) + i));
                if (return_as_table) { lua_rawset(lua_state, -3); }
            }
            return return_as_table ? 1 : num_bytes_to_read;
        }
    }

    template<typename NumberType, typename LuaVerificationFunction, typename LuaFetchFunction>
    auto write_generic_number_wrapper(lua_State* lua_state, LuaVerificationFunction lua_verification_function, LuaFetchFunction lua_fetch_function, ByteReadWriteMode byte_read_write_mode = ByteReadWriteMode::SingleByte) -> int
    {
        NumberType* data{};
        NumberType new_value{};

        if (lua_isinteger(lua_state, 1))
        {
            data = std::bit_cast<NumberType*>(lua_tointeger(lua_state, 1));
        }
        else if (lua_islightuserdata(lua_state, 1))
        {
            data = static_cast<NumberType*>(lua_touserdata(lua_state, 1));
        }
        else if (lua_isuserdata(lua_state, 1))
        {
            auto get_address_type = lua_getfield(lua_state, 1, "GetAddress");
            if (get_address_type != LUA_TFUNCTION)
            {
                luaL_error(lua_state, "Userdata is incompatible with memory functions because it has has no 'GetAddress' function.");
            }

            lua_pushvalue(lua_state, 1); // Duplicate the userdata because pcall will pop it from the stack for the first function param (self).

            if (int status = lua_pcall(lua_state, 1, 1, 0); status != LUA_OK)
            {
                luaL_error(lua_state, LuaBindings::resolve_status_message(lua_state, status).c_str());
            }

            if (!lua_isinteger(lua_state, -1) && !lua_isnil(lua_state, -1))
            {
                luaL_error(lua_state, "Userdata is incompatible with memory functions because its 'GetAddress' function returned non-integer.");
            }

            if (lua_isnil(lua_state, -1))
            {
                luaL_error(lua_state, "Cannot place a call to a memory function with a nullptr.");
            }

            data = std::bit_cast<NumberType*>(lua_tointeger(lua_state, -1));
            lua_pop(lua_state, 1);
        }

        bool values_as_table = lua_istable(lua_state, 2);

        if (byte_read_write_mode == ByteReadWriteMode::SingleByte)
        {
            if (values_as_table)
            {
                lua_pushinteger(lua_state, 1);
                lua_rawget(lua_state, 2);
            }
            if (!lua_verification_function(lua_state, values_as_table ? -1 : 2))
            {
                luaL_error(lua_state, "The new value is invalid for this type.");
            }
            new_value = lua_fetch_function(lua_state, values_as_table ? -1 : 2);
            *data = new_value;
        }
        else
        {
            if (values_as_table)
            {
                lua_pushnil(lua_state);
                for (lua_Integer i = 0; lua_next(lua_state, 2); ++i)
                {
                    if (lua_isinteger(lua_state, -1))
                    {
                        new_value = lua_fetch_function(lua_state, -1);
                        data[i] = new_value;
                    }
                    lua_pop(lua_state, 1);
                }
            }
            else
            {
                for (lua_Integer i = 2; lua_verification_function(lua_state, i); ++i)
                {
                    new_value = lua_fetch_function(lua_state, i);
                    data[i - 2] = new_value;
                }
            }
        }

        return 1;
    }

    auto lua_fetch_integer(lua_State* lua_state, int index) -> int
    {
        return lua_tointeger(lua_state, index);
    }

    auto lua_Test_ReadBytes_wrapper(lua_State* lua_state) -> int
    {
        return read_generic_number_wrapper<uint8_t>(lua_state, ByteReadWriteMode::MultiByte);
    }
    auto lua_Test_ReadUInt8_wrapper(lua_State* lua_state) -> int
    {
        return lua_isboolean(lua_state, 2) && lua_toboolean(lua_state, 2) ? read_generic_number_wrapper<int8_t>(lua_state) : read_generic_number_wrapper<uint8_t>(lua_state);
    }
    auto lua_Test_ReadUInt16_wrapper(lua_State* lua_state) -> int
    {
        return read_generic_number_wrapper<uint16_t>(lua_state);
    }
    auto lua_Test_ReadUInt32_wrapper(lua_State* lua_state) -> int
    {
        return read_generic_number_wrapper<uint32_t>(lua_state);
    }
    auto lua_Test_ReadUInt64_wrapper(lua_State* lua_state) -> int
    {
        return read_generic_number_wrapper<uint64_t>(lua_state);
    }
    auto lua_Test_ReadInt8_wrapper(lua_State* lua_state) -> int
    {
        return read_generic_number_wrapper<int8_t>(lua_state);
    }
    auto lua_Test_ReadInt16_wrapper(lua_State* lua_state) -> int
    {
        return read_generic_number_wrapper<int16_t>(lua_state);
    }
    auto lua_Test_ReadInt32_wrapper(lua_State* lua_state) -> int
    {
        return read_generic_number_wrapper<int32_t>(lua_state);
    }
    auto lua_Test_ReadInt64_wrapper(lua_State* lua_state) -> int
    {
        return read_generic_number_wrapper<int64_t>(lua_state);
    }

    auto lua_Test_WriteBytes_wrapper(lua_State* lua_state) -> int
    {
        return write_generic_number_wrapper<uint8_t>(lua_state, &lua_isinteger, &lua_fetch_integer, ByteReadWriteMode::MultiByte);
    }
    auto lua_Test_WriteUInt8_wrapper(lua_State* lua_state) -> int
    {
        return write_generic_number_wrapper<uint8_t>(lua_state, &lua_isinteger, &lua_fetch_integer);
    }
    auto lua_Test_WriteUInt16_wrapper(lua_State* lua_state) -> int
    {
        return write_generic_number_wrapper<uint16_t>(lua_state, &lua_isinteger, &lua_fetch_integer);
    }
    auto lua_Test_WriteUInt32_wrapper(lua_State* lua_state) -> int
    {
        return write_generic_number_wrapper<uint32_t>(lua_state, &lua_isinteger, &lua_fetch_integer);
    }
    auto lua_Test_WriteUInt64_wrapper(lua_State* lua_state) -> int
    {
        return write_generic_number_wrapper<uint64_t>(lua_state, &lua_isinteger, &lua_fetch_integer);
    }

    auto lua_Test_WriteInt8_wrapper(lua_State* lua_state) -> int
    {
        return write_generic_number_wrapper<int8_t>(lua_state, &lua_isinteger, &lua_fetch_integer);
    }
    auto lua_Test_WriteInt16_wrapper(lua_State* lua_state) -> int
    {
        return write_generic_number_wrapper<int16_t>(lua_state, &lua_isinteger, &lua_fetch_integer);
    }
    auto lua_Test_WriteInt32_wrapper(lua_State* lua_state) -> int
    {
        return write_generic_number_wrapper<int32_t>(lua_state, &lua_isinteger, &lua_fetch_integer);
    }
    auto lua_Test_WriteInt64_wrapper(lua_State* lua_state) -> int
    {
        return write_generic_number_wrapper<int64_t>(lua_state, &lua_isinteger, &lua_fetch_integer);
    }
}
