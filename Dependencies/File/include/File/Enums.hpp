#ifndef RC_FILE_ENUMS_HPP
#define RC_FILE_ENUMS_HPP

namespace RC::File
{
    enum class OpenFor
    {
        Writing,
        Appending,
        Reading,
    };

    enum class OverwriteExistingFile
    {
        Yes,
        No,
    };

    enum class CreateIfNonExistent
    {
        Yes,
        No,
    };

    enum class IsCachedHandle
    {
        Yes,
        No,
    };

    enum class GenericDataType
    {
        UnsignedLong,
        SignedLong,
        UnsignedLongLong,
        SignedLongLong,
    };
}

#endif //RC_FILE_ENUMS_HPP
