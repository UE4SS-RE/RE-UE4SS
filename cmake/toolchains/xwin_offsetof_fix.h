#pragma once
// Fix for offsetof macro incompatibility with ImGui and Windows SDK
#ifdef offsetof
#undef offsetof
#endif
#define offsetof(type, member) __builtin_offsetof(type, member)