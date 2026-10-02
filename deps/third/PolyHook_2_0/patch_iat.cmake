function(ue4ss_patch_polyhook_iat source_directory)
    set(source_file "${source_directory}/sources/IatHook.cpp")
    file(READ "${source_file}" source)

    set(original [=[		// Original holds the API Names
        auto pOriginalThunk = (PIMAGE_THUNK_DATA)
			RVA2VA(uintptr_t, moduleBase, pImports[i].OriginalFirstThunk);

		// FirstThunk is overwritten by loader with API addresses, we change this
        auto pThunk = (PIMAGE_THUNK_DATA)
			RVA2VA(uintptr_t, moduleBase, pImports[i].FirstThunk);

		if (!pOriginalThunk) {
			Log::log("IAT's without valid original thunk are un-supported", ErrorLevel::SEV);
			return nullptr;
		}]=])

    set(replacement [=[		// A zero RVA becomes moduleBase, not nullptr. A bound IAT alone has no API names.
		if (pImports[i].OriginalFirstThunk == 0 || pImports[i].FirstThunk == 0) {
			Log::log("Skipping IAT descriptor without an original lookup or address table", ErrorLevel::SEV);
			continue;
		}

		// Original holds the API Names
        auto pOriginalThunk = (PIMAGE_THUNK_DATA)
			RVA2VA(uintptr_t, moduleBase, pImports[i].OriginalFirstThunk);

		// FirstThunk is overwritten by loader with API addresses, we change this
        auto pThunk = (PIMAGE_THUNK_DATA)
			RVA2VA(uintptr_t, moduleBase, pImports[i].FirstThunk);]=])

    string(FIND "${source}" "${replacement}" patched_position)
    if(NOT patched_position EQUAL -1)
        return()
    endif()
    string(FIND "${source}" "${original}" original_position)
    if(original_position EQUAL -1)
        message(FATAL_ERROR "PolyHook IAT source changed; review the missing-thunk patch: ${source_file}")
    endif()

    string(REPLACE "${original}" "${replacement}" source "${source}")
    file(WRITE "${source_file}" "${source}")
    message(STATUS "Patched PolyHook IAT missing-table validation")
endfunction()