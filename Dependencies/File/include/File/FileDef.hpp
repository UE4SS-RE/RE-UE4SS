#ifndef RC_FILE_FILEDEF_HPP
#define RC_FILE_FILEDEF_HPP

namespace RC::File
{
#ifndef RC_DETECTED_OS
#ifdef _WIN32
#define RC_DETECTED_OS _WIN32
#else
    static_assert(false, "Could not setup the 'Handle' typedef because a supported OS was not detected.");
#endif
#endif

#if RC_DETECTED_OS == _WIN32
#ifndef RC_OS_FILE_TYPE_INCLUDE_FILE
#define RC_OS_FILE_TYPE_INCLUDE_FILE <File/FileType/WinFile.hpp>
#else
    static_assert(false, "Could not setup the 'RC_OS_FILE_TYPE_INCLUDE_FILE' macro because a supported OS was not detected.");
#endif
#endif

}

#endif //RC_FILE_FILEDEF_HPP
