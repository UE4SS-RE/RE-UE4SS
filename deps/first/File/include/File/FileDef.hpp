#pragma once

namespace RC::File
{
#ifndef RC_DETECTED_OS
#ifdef _WIN32
#define RC_DETECTED_OS _WIN32
#elif __linux__
#define RC_DETECTED_OS __linux__
#else
    static_assert(false, "Could not setup the 'Handle' typedef because a supported OS was not detected.");
#endif
#endif

#ifndef RC_OS_FILE_TYPE_INCLUDE_FILE
#if RC_DETECTED_OS == _WIN32
#define RC_OS_FILE_TYPE_INCLUDE_FILE <File/FileType/WinFile.hpp>
#elif RC_DETECTED_OS == __linux__
#define RC_OS_FILE_TYPE_INCLUDE_FILE <File/FileType/LinuxFile.hpp>
#else
    static_assert(false, "Could not setup the 'RC_OS_FILE_TYPE_INCLUDE_FILE' macro because a supported OS was not detected.");
#endif
#endif

} // namespace RC::File
