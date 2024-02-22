package("raw_pdb")
    add_urls("git@github.com:MolecularMatters/raw_pdb.git")
    add_urls("https://github.com/MolecularMatters/raw_pdb.git")

    add_versions("v1.0.0", "8c6a7146393c83d27fa101e8bc8017f2a7f151df")

    add_deps("cmake")
    
    on_install(function (package)
        local configs = {}
        table.insert(configs, "-DCMAKE_BUILD_TYPE=" .. (package:debug() and "Debug" or "Release"))
        table.insert(configs, "-DBUILD_SHARED_LIBS=OFF")
        import("package.tools.cmake").install(package, configs)

        os.cp("src/*.h", package:installdir("include"))
        os.cp("src/Foundation/*.h", package:installdir("include/Foundation"))
    end)
package_end()