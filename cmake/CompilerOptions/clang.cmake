set(DEFAULT_COMPILER_FLAGS "-g;-gcodeview;-fcolor-diagnostics;-Wno-unknown-pragmas;-Wno-unused-parameter;-fms-extensions;-Wignored-attributes" PARENT_SCOPE)

set(LINKER_FLAGS "-g" PARENT_SCOPE)
set(DEFAULT_SHARED_LINKER_FLAGS "${LINKER_FLAGS}" PARENT_SCOPE)
set(DEFAULT_EXE_LINKER_FLAGS "${LINKER_FLAGS}" PARENT_SCOPE)

set(Shipping_FLAGS "" PARENT_SCOPE)