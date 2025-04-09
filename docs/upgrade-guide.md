# RE-UE4SS Upgrade Guide

This document provides detailed guidance for upgrading between versions of RE-UE4SS. Each section covers a specific version upgrade and describes breaking changes, deprecations, and the steps required to migrate your code successfully.

## Version 3.x to 4.x

> **Released:** [TBD]  
> **PR:** [Link to PR when available]

### Breaking Changes

#### TObjectPtr Implementation Change
**What Changed:**  
The `TObjectPtr<>` class has been enhanced to function as a proper smart pointer instead of a simple wrapper.

**Before (v3.x):**
```cpp
// Simple class that makes everything compile.
template<typename UnderlyingType>
class TObjectPtr
{
public:
    UnderlyingType* UnderlyingObjectPointer;
};
```

**After (v4.x):**
```cpp
template<typename T>
class TObjectPtr
{
public:
    using ElementType = T;
    // Constructors
    TObjectPtr() : Ptr(nullptr) {}
    TObjectPtr(TYPE_OF_NULLPTR) : Ptr(nullptr) {}
    TObjectPtr(const TObjectPtr& Other) : Ptr(Other.Ptr) {}
    explicit TObjectPtr(ElementType* InPtr) : Ptr(InPtr) {}

    // Assignment operators
    TObjectPtr& operator=(const TObjectPtr& Other) { Ptr = Other.Ptr; return *this; }
    TObjectPtr& operator=(TYPE_OF_NULLPTR) { Ptr = nullptr; return *this; }
    TObjectPtr& operator=(ElementType* InPtr) { Ptr = InPtr; return *this; }
        
    // Conversion operators
    FORCEINLINE operator T* () const { return Get(); }
    template <typename U>
    UE_OBJPTR_DEPRECATED(5.0, "Explicit cast to other raw pointer types is deprecated.  Please use the Cast API or get the raw pointer with ToRawPtr and cast that instead.")
    explicit FORCEINLINE operator U* () const { return (U*)Get(); }
    explicit FORCEINLINE operator UPTRINT() const { return (UPTRINT)Get(); }
    FORCEINLINE T* operator->() const { return Get(); }
    FORCEINLINE T& operator*() const { return *Get(); }

    // Comparison operators
    bool operator==(const TObjectPtr& Other) const { return Ptr == Other.Ptr; }
    bool operator!=(const TObjectPtr& Other) const { return Ptr != Other.Ptr; }
    bool operator==(const ElementType* InPtr) const { return Ptr == InPtr; }
    bool operator!=(const ElementType* InPtr) const { return Ptr != InPtr; }
    bool operator==(TYPE_OF_NULLPTR) const { return Ptr == nullptr; }
    bool operator!=(TYPE_OF_NULLPTR) const { return Ptr != nullptr; }

    // Additional API compatibility with UE's TObjectPtr
    bool operator!() const { return Ptr == nullptr; }
    explicit operator bool() const { return Ptr != nullptr; }
    ElementType* Get() const { return Ptr; }

private:
    ElementType* Ptr;
};
```

**Migration Steps:**
1. For direct pointer access (previously accessed via `UnderlyingObjectPointer`):
   ```cpp
   // Old
   TObjectPtr<UClass> ClassPtr;
   UClass* RawPtr = ClassPtr.UnderlyingObjectPointer;
   
   // New
   TObjectPtr<UClass> ClassPtr;
   UClass* RawPtr = ClassPtr; // implicit conversion
   // or
   UClass* RawPtr = ClassPtr.Get(); // explicit access
   ```

2. For address-of operations on TObjectPtr contents:
   ```cpp
   // Old
   &ObjectPtr.UnderlyingObjectPointer
   
   // New
   &ObjectPtr.Get()
   // or simply
   &ObjectPtr
   ```

3. For pointer-to-integer conversions (e.g., formatting addresses):
    ```cpp
    // Old
    uintptr_t addr = reinterpret_cast<uintptr_t>(ObjectPtr.UnderlyingObjectPointer);

    // New
    uintptr_t addr = reinterpret_cast<uintptr_t>(ObjectPtr.Get());
    // or
    uintptr_t addr = reinterpret_cast<uintptr_t>(ToRawPtr(ObjectPtr)); // using helper to extract a raw pointer from TObjectPtr
    ```

#### FString API Changes

##### `GetCharArray()` Behavior Change

**What Changed:**  
The `GetCharArray()` method now returns the actual array instead of a pointer to characters.

**Before (v3.x):**
```cpp
// Returned a character pointer
auto GetCharArray() const -> const CharType*;
```

**After (v4.x):**
```cpp
// Returns the array itself
FORCEINLINE DataType& GetCharArray() { return Data; }
FORCEINLINE const DataType& GetCharArray() const { return Data; }

// Use operator* for character pointer access
FORCEINLINE const TCHAR* operator*() const { return Data.Num() ? Data.GetData() : TEXT(""); }
```

**Migration Steps:**

1. For character pointer access (previously `GetCharArray()`):
   ```cpp
   // Old
   const TCHAR* ptr = myString.GetCharArray();
   
   // New
   const TCHAR* ptr = *myString;
   ```

2. For array access (previously `GetCharTArray()`):
   ```cpp
   // Old
   TArray<TCHAR>& arr = myString.GetCharTArray();
   
   // New
   TArray<TCHAR>& arr = myString.GetCharArray();
   ```

### Deprecations

- `GetCharTArray()` is now deprecated but still available for compatibility.
  It forwards to `GetCharArray()` and will be removed in a future version.

## Reporting Migration Issues

If you encounter problems while upgrading, please:

1. Check the [open issues](https://github.com/UE4SS-RE/RE-UE4SS/issues) for similar reports
2. Create a new upgrade problem issue if your issue hasn't been reported
