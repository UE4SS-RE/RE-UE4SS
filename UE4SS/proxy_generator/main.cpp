#include <Constructs/Views/EnumerateView.hpp>
#include <File/File.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <ImageHlp.h>
#include <tchar.h>

using namespace RC;
namespace fs = std::filesystem;

using std::cerr;
using std::cout;
using std::endl;
using std::ofstream;

using std::string;

struct ExportFunction
{
    uint16_t ordinal;
    bool is_named;
    string name;

    ExportFunction(uint16_t ordinal, bool is_named, string name) : ordinal(ordinal), is_named(is_named), name(name)
    {
    }
};

std::vector<ExportFunction> DumpExports(const fs::path& dll_path)
{
    auto dll_file = File::open(dll_path);
    const auto dll_file_map = dll_file.memory_map();

    ULONG export_directory_size = 0;
    IMAGE_EXPORT_DIRECTORY* export_directory =
            (IMAGE_EXPORT_DIRECTORY*)ImageDirectoryEntryToData(dll_file_map.data(), FALSE, IMAGE_DIRECTORY_ENTRY_EXPORT, &export_directory_size);

    if (export_directory == nullptr)
    {
        cerr << std::format("Failed to get export directory, reason: {:x}", GetLastError()) << endl;
        return {};
    }

    IMAGE_DOS_HEADER* dos_header = (IMAGE_DOS_HEADER*)dll_file_map.data();
    IMAGE_NT_HEADERS* nt_header = (IMAGE_NT_HEADERS*)(dll_file_map.data() + dos_header->e_lfanew);

    DWORD* name_rvas = (DWORD*)ImageRvaToVa(nt_header, dll_file_map.data(), export_directory->AddressOfNames, NULL);
    DWORD* function_rvas = (DWORD*)ImageRvaToVa(nt_header, dll_file_map.data(), export_directory->AddressOfFunctions, NULL);
    uint16_t* ordinals = (uint16_t*)ImageRvaToVa(nt_header, dll_file_map.data(), export_directory->AddressOfNameOrdinals, NULL);

    std::vector<ExportFunction> exports;
    std::set<uint16_t> exported_ordinals;

    for (size_t i = 0; i < export_directory->NumberOfNames; i++)
    {
        std::string export_name = (char*)ImageRvaToVa(nt_header, dll_file_map.data(), name_rvas[i], NULL);
        uint16_t ordinal = ordinals[i] + 1;

        ExportFunction named_export(ordinal, true, export_name);
        exports.push_back(named_export);

        exported_ordinals.insert(ordinal);
    }

    for (size_t i = 0; i < export_directory->NumberOfFunctions; i++)
    {
        uint16_t ordinal = (uint16_t)(export_directory->Base + i);
        uint32_t function_rva = function_rvas[i];

        if (function_rva == 0) continue;
        if (exported_ordinals.contains(ordinal)) continue; // a named function for this ordinal was already exported

        ExportFunction ordinal_export(ordinal, false, std::format("ordinal{}", ordinal));
        exports.push_back(ordinal_export);
    }

    return exports;
}

int _tmain(int argc, TCHAR* argv[])
{
    if (argc != 3)
    {
        cerr << "Invalid arguments! Expected: proxy_generator.exe <input_dll_name> <output_path>" << endl;
        return -1;
    }

    const fs::path input_dll = argv[1];
    const fs::path input_dll_name = input_dll.filename();
    const fs::path output_path = argv[2];

    if (!fs::exists(input_dll))
    {
        cerr << "Input dll doesn't exist!\n" << endl;
        return -1;
    }

    cout << std::format("Generating a proxy for {}, output path: {}", input_dll.string(), output_path.string()) << endl;

    const auto exports = DumpExports(input_dll);
    cout << std::format("Export count: {}", exports.size()) << endl;

    ofstream def_file((output_path / input_dll_name).replace_extension("def"));
    def_file << std::format("LIBRARY {}", fs::path(input_dll_name).replace_extension().string()) << endl;
    def_file << "EXPORTS" << endl;

    for (const auto [e, index] : exports | views::enumerate)
    {
        def_file << std::format("  {}=f{} @{}", e.name, index, e.ordinal) << endl;
    }
    def_file.close();

    ofstream asm_file((output_path / input_dll_name).replace_extension("asm"));
    asm_file << ".code" << endl;
    asm_file << "extern mProcs:QWORD" << endl;

    for (const auto [e, index] : exports | views::enumerate)
    {
        asm_file << std::format("f{} proc", index) << endl;
        asm_file << std::format("  jmp mProcs[8*{}]", index) << endl;
        asm_file << std::format("f{} endp", index) << endl;
    }

    asm_file << "end" << endl;
    asm_file.close();

    ofstream cpp_file(output_path / "dllmain.cpp");
    cpp_file << "#include <File/Macros.hpp>" << endl;
    cpp_file << endl;
    cpp_file << "#include <cstdint>" << endl;
    cpp_file << endl;
    cpp_file << "#define WIN32_LEAN_AND_MEAN" << endl;
    cpp_file << "#include <Windows.h>" << endl;
    cpp_file << endl;

    cpp_file << "using namespace RC;" << endl;
    cpp_file << endl;

    cpp_file << "HMODULE SOriginalDll = nullptr;" << endl;
    cpp_file << std::format("extern \"C\" uintptr_t mProcs[{}] = {{0}};", exports.size()) << endl;

    cpp_file << "void setup_functions()" << endl;
    cpp_file << "{" << endl;

    for (const auto [e, index] : exports | views::enumerate)
    {
        string getter = e.is_named ? std::format("\"{}\"", e.name) : std::format("MAKEINTRESOURCEA({})", e.ordinal);
        cpp_file << std::format("    mProcs[{}] = (uintptr_t)GetProcAddress(SOriginalDll, {});", index, getter) << endl;
    }

    cpp_file << "}" << endl;
    cpp_file << endl;

    cpp_file << "void load_original_dll()" << endl;
    cpp_file << "{" << endl;
    cpp_file << "    SystemCharType path[MAX_PATH];" << endl;
    cpp_file << "    GetSystemDirectory(path, MAX_PATH);" << endl;
    cpp_file << endl;
    cpp_file << std::format("    SystemStringType dll_path = SystemStringType(path) + SYSSTR(\"\\\\{}\");", input_dll_name.string()) << endl;
    cpp_file << endl;
    cpp_file << "    SOriginalDll = LoadLibrary(dll_path.c_str());" << endl;
    cpp_file << "    if (!SOriginalDll)" << endl;
    cpp_file << "    {" << endl;
    cpp_file << "        ExitProcess(0);" << endl;
    cpp_file << "    }" << endl;
    cpp_file << "}" << endl;
    cpp_file << endl;

    cpp_file << "BOOL WINAPI DllMain(HMODULE hInstDll, DWORD fdwReason, LPVOID lpvReserved)" << endl;
    cpp_file << "{" << endl;
    cpp_file << "    if (fdwReason == DLL_PROCESS_ATTACH)" << endl;
    cpp_file << "    {" << endl;
    cpp_file << "        load_original_dll();" << endl;
    cpp_file << "        setup_functions();" << endl;
    cpp_file << "        LoadLibrary(SYSSTR(\"UE4SS.dll\"));" << endl;
    cpp_file << "    }" << endl;
    cpp_file << "    else if (fdwReason == DLL_PROCESS_DETACH)" << endl;
    cpp_file << "    {" << endl;
    cpp_file << "        FreeLibrary(SOriginalDll);" << endl;
    cpp_file << "    }" << endl;
    cpp_file << "    return TRUE;" << endl;
    cpp_file << "}" << endl;

    cpp_file.close();

    cout << "Finished generating!" << endl;

    return 0;
}
