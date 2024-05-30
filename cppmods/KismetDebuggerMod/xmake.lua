local projectName = "KismetDebuggerMod"

target(projectName)
    add_rules("ue4ss.mod")
	add_includedirs("include")
	add_files("src/dllmain.cpp", "src/KismetDebugger.cpp")