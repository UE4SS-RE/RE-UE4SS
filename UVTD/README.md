# RE-UVTD

I am not the original author of this tool.  I am rehosting it with permission.
 
### Prerequisites:
Visual Studio 2022


CMake

### Build Instructions:
1. Clone the repository
2. Open Command Prompt in the repository folder
3. Run the following command:
    cmake CMakeLists.txt
4. Open the generated solution file in Visual Studio
5. Build the solution

### Run Instructions:
1. Copy the file "msdia140.dll" from "C:\Program Files (x86)\Microsoft Visual Studio\2022\Community\Common7\IDE" or your VS installation path to the folder where the executable is located
2. Create a folder named "PDBs" in the same folder as the executable
3. Copy the PDB files of the UE engine versions you want to analyze to the "PDBs" folder.  PDBs from source engine builds are preferred
4. Run the executable
5. Follow the directions in the console window

### Adding additional types to dump:
Add the class or struct name to the following variables and function implementations in the format used for each
    static std::vector<ObjectItem> s_object_items
    static std::unordered_set<File::StringType> valid_udt_names
    auto static is_valid_type_to_dump(File::StringType type_name) -> bool

### Adding additional UE engine versions:
1. Copy the PDB files of the UE engine versions you want to analyze to the "PDBs" folder.  PDBs from source engine builds are preferred
2. Go to the function implementation for main() found at line 1672 in UVTD_DIA.cpp
3. Add the following code with the naming of your PDB file
    TRY([&] {
            {
                VTableDumper vtable_dumper{"PDBs/4_XX.pdb"};
                vtable_dumper.generate_code(vtable_or_member_vars);
            }
            CoUninitialize();
        });



### Todo
Make types to dump be gathered from a config file rather than through editing source

 
Other cleanup
