#pragma once

struct lua_State;

namespace RC
{
    struct MemoryItem
    {
    };

    // Native functions
    auto AllocateMemory(size_t size) -> MemoryItem*;
    auto FreeMemory(MemoryItem* memory_item) -> void;
    auto FreeMemory(uintptr_t memory) -> void;

    // Custom wrappers
    auto lua_Test_ReadBytes_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_ReadUInt8_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_ReadUInt16_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_ReadUInt32_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_ReadUInt64_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_ReadInt8_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_ReadInt16_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_ReadInt32_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_ReadInt64_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_WriteBytes_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_WriteUInt8_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_WriteUInt16_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_WriteUInt32_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_WriteUInt64_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_WriteInt8_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_WriteInt16_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_WriteInt32_wrapper(lua_State* lua_state) -> int;
    auto lua_Test_WriteInt64_wrapper(lua_State* lua_state) -> int;
} // namespace RC
