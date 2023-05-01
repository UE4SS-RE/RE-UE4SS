#ifndef RC_FILE_INTERNALFILE_HPP
#define RC_FILE_INTERNALFILE_HPP

#include <File/Common.hpp>
#include <File/Enums.hpp>

namespace RC::File
{
    namespace Internal
    {
        struct StaticStorage
        {
            static inline bool internal_error{};
        };
    }

    struct RC_FILE_API OpenProperties
    {
        OpenFor open_for;
        OverwriteExistingFile overwrite_existing_file;
        CreateIfNonExistent create_if_non_existent;
    };

    struct RC_FILE_API GenericItemData
    {
        GenericDataType data_type;

        union
        {
            unsigned long data_ulong;
            signed long data_long;
            unsigned long long data_ulonglong;
            signed long long data_longlong;
        };
    };
}

#endif //RC_FILE_INTERNALFILE_HPP
