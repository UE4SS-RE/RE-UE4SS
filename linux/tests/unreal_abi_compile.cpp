#include <cstddef>
#include <type_traits>

#include <Unreal/Core/HAL/Platform.hpp>
#include <Unreal/NameTypes.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/SoftObjectPtr.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/FURL.hpp>
#include <Unreal/CoreUObject/UObject/UnrealType.hpp>

int main()
{
    using namespace RC::Unreal;

    static_assert(PLATFORM_LINUX == 1);
    static_assert(PLATFORM_UNIX == 1);
    static_assert(PLATFORM_64BITS == 1);
    static_assert(sizeof(void*) == 8);
    static_assert(sizeof(TCHAR) == 2);
    static_assert(sizeof(WIDECHAR) == 2);
    static_assert(sizeof(wchar_t) == 4);
    static_assert(std::is_same_v<TCHAR, char16_t>);
    static_assert(std::is_same_v<WIDECHAR, char16_t>);
    static_assert(sizeof(SIZE_T) == sizeof(void*));
    static_assert(sizeof(FNameEntryId) == 4);
    static_assert(sizeof(FName) == 8);
    static_assert(alignof(FName) == 4);
    static_assert(std::is_class_v<FName>);
    static_assert(std::is_class_v<UObject>);
    static_assert(std::is_class_v<UFunction>);
    static_assert(std::is_class_v<FProperty>);
    static_assert(offsetof(FSoftObjectPath, AssetPathName) == 0x0);
    static_assert(offsetof(FSoftObjectPath, AssetName) == 0x8);
    static_assert(offsetof(FSoftObjectPath, SubPathString) == 0x10);
    static_assert(sizeof(FSoftObjectPath) == 0x20);
    static_assert(sizeof(FSoftObjectPtr) == 0x30);
    static_assert(sizeof(FScriptInterface) == 0x10);
    static_assert(sizeof(FText) == 0x18);
    static_assert(sizeof(FURL) == 0x68);
    static_assert(offsetof(FURL, Protocol) == 0x00);
    static_assert(offsetof(FURL, Host) == 0x10);
    static_assert(offsetof(FURL, Port) == 0x20);
    static_assert(offsetof(FURL, Valid) == 0x24);
    static_assert(offsetof(FURL, Map) == 0x28);
    static_assert(offsetof(FURL, RedirectURL) == 0x38);
    static_assert(offsetof(FURL, Op) == 0x48);
    static_assert(offsetof(FURL, Portal) == 0x58);

    TCHAR Buffer[16]{};
    FPlatformString::Strcpy(Buffer, 16, TEXT("PalServer"));
    if (FPlatformString::Strlen(Buffer) != 9 ||
        FPlatformString::Strcmp(Buffer, TEXT("PalServer")) != 0 ||
        FPlatformString::Atoi(TEXT("100427")) != 100427)
    {
        return 1;
    }

    volatile int64 Counter = 1;
    if (FPlatformAtomics::InterlockedIncrement(&Counter) != 2 ||
        FPlatformAtomics::InterlockedAdd(&Counter, 3) != 2 ||
        FPlatformAtomics::AtomicRead(&Counter) != 5 ||
        FPlatformAtomics::InterlockedCompareExchange(&Counter, 7, 5) != 5 ||
        FPlatformAtomics::AtomicRead(&Counter) != 7)
    {
        return 2;
    }
    return 0;
}
