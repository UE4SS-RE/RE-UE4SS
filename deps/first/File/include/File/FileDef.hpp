#pragma once

namespace RC::File
{
#ifndef RC_DETECTED_OS
#ifdef WIN32
#ifdef _WIN32
#define RC_DETECTED_OS _WIN32
#else
    static_assert(false, "Could not setup the 'Handle' typedef because a supported OS was not detected.");
#endif

#else

#ifdef LINUX
#define RC_DETECTED_OS LINUX
#else
    static_assert(false, "Could not setup the 'Handle' typedef because a supported OS was not detected.");
#endif

#endif
#endif

#if RC_DETECTED_OS == LINUX
#ifndef RC_OS_FILE_TYPE_INCLUDE_FILE
#define RC_OS_FILE_TYPE_INCLUDE_FILE <File/FileType/StdFile.hpp>
#else
    static_assert(false, "Could not setup the 'RC_OS_FILE_TYPE_INCLUDE_FILE' macro because a supported OS was not detected.");
#endif
#endif
#if RC_DETECTED_OS == _WIN32
#ifndef RC_OS_FILE_TYPE_INCLUDE_FILE
#define RC_OS_FILE_TYPE_INCLUDE_FILE <File/FileType/WinFile.hpp>
#else
    static_assert(false, "Could not setup the 'RC_OS_FILE_TYPE_INCLUDE_FILE' macro because a supported OS was not detected.");
#endif
#endif

} // namespace RC::File
