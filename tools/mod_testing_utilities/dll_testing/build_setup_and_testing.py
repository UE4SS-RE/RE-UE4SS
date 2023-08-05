import os
import sys
import shutil
import subprocess

# These are the ones you need to manually specify

# This is the steam app id for the game, so you can launch into it properly, instead of through exe
# Example"
# steam_game_app_id = 12345
steam_game_app_id = 1282100

# This is for if you want to compile the project in Debug versus Release
# Example:
# build_type = "Debug"
# build_type = "Release"
build_type = "Debug"

# This is the name of the game's exe in the Binaries/Win64 directory
# Example:
# game_project_name = r"D:/SteamLibrary/steamapps/common/Lies of P Demo/LiesofP/Content/Binaries/Win64/LOP-Win64-Shipping.exe"
game_exe = r"C:/Program Files (x86)/Steam/steamapps/common/Remnant2/Remnant2/Binaries/Win64/Remnant2-Win64-Shipping.exe"

# This is the path to your MSBuild.exe from VS 2022, that will be used for compiling
# Example:
# ms_build_path = r"C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe"
ms_build_path = r"C:/Program Files/Microsoft Visual Studio/2022/Community/MSBuild/Current/Bin/MSBuild.exe"


# This is for if you want to run the game through steam appid, or through the exe directly
# Example:
# game_run_type = "win_64_exe"
# game_run_type = "steam"
game_run_type = "win_64_exe"

# Code below this shouldn't need manual changes

# Changes the working dir to the one the script is in
script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)
os.chdir("../../../..")

latest_path = ""
latest_version = ""
my_dll_mods_dir = os.getcwd()
output_dir = f"{my_dll_mods_dir}/{build_type}Output"
solution = f"{my_dll_mods_dir}/{build_type}Output/MyDllMods.sln"
game_dir = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(game_exe))))
game_project_name = os.path.dirname((os.path.dirname(os.path.dirname(game_exe))))

old_ue4ss_ini = f"{game_project_name}/Binaries/Win64/UE4SS-settings.ini"
new_ue4ss_ini = f"{my_dll_mods_dir}/RE-UE4SS/assets/UE4SS-settings.ini"

old_mods_dir = f"{game_project_name}/Binaries/Win64/Mods"
new_mods_dir = f"{my_dll_mods_dir}/RE-UE4SS/assets/Mods"

old_ue4ss_xinput_dll = f"{game_project_name}/Binaries/Win64/xinput1_3.dll"
old_ue4ss_xinput_pdb = f"{game_project_name}/Binaries/Win64/xinput1_3.pdb"

old_ue4ss_dll = f"{game_project_name}/Binaries/Win64/ue4ss.dll"
old_ue4ss_pdb = f"{game_project_name}/Binaries/Win64/ue4ss.pdb"

new_ue4ss_dll = f"{my_dll_mods_dir}/Output/ue4ss/Binaries/x64/{build_type}/ue4ss.dll"
new_ue4ss_pdb = f"{my_dll_mods_dir}/Output/ue4ss/Binaries/x64/{build_type}/ue4ss.dll"

old_example_mod_dll = f"{game_project_name}/Binaries/Win64/Mods/ExampleMod/dlls/main.dll"
old_example_mod_pdb = f"{game_project_name}/Binaries/Win64/Mods/ExampleMod/dlls/main.pdb"
old_example_mod_enabled_txt = f"{game_project_name}/Binaries/Win64/Mods/ExampleMod/enabled.txt"

new_ue4ss_xinput_dll = f"{my_dll_mods_dir}/Output/ue4ss/Binaries/x64/{build_type}/xinput1_3.dll"
new_ue4ss_xinput_pdb = f"{my_dll_mods_dir}/Output/ue4ss/Binaries/x64/{build_type}/xinput1_3.dll"

new_example_mod_dll = f"{my_dll_mods_dir}/{build_type}Output/ExampleMod/{build_type}/ExampleMod.dll"
new_example_mod_pdb = f"{my_dll_mods_dir}/{build_type}Output/ExampleMod/{build_type}/ExampleMod.pdb"

template_my_dll_mods_cmake_list = f"{my_dll_mods_dir}/RE-UE4SS/tools/mod_testing_utilities/assets/CMakeLists.txt"
template_my_example_mod_cmake_list = f"{my_dll_mods_dir}/RE-UE4SS/tools/mod_testing_utilities/assets/ExampleMod/CMakeLists.txt"
template_dllmain = f"{my_dll_mods_dir}/RE-UE4SS/tools/mod_testing_utilities/assets/ExampleMod/dllmain.cpp"

if not os.path.isdir(f"{my_dll_mods_dir}/ExampleMod"):
    os.mkdir(f"{my_dll_mods_dir}/ExampleMod")

if not os.path.isdir(f"{game_project_name}/Binaries/Win64/Mods/ExampleMod"):
    os.mkdir(f"{game_project_name}/Binaries/Win64/Mods/ExampleMod")

if not os.path.isdir(f"{game_project_name}/Binaries/Win64/Mods/ExampleMod/dlls"):
    os.mkdir(f"{game_project_name}/Binaries/Win64/Mods/ExampleMod/dlls")

if not os.path.isfile(old_example_mod_enabled_txt):
    open(old_example_mod_enabled_txt, 'w').close()

if not os.path.isfile("CMakeLists.txt"):
    shutil.copy(template_my_dll_mods_cmake_list, f"{my_dll_mods_dir}/CMakeLists.txt")

if not os.path.isfile("ExampleMod/CMakeLists.txt"):
    shutil.copy(template_my_example_mod_cmake_list, f"{my_dll_mods_dir}/ExampleMod/CMakeLists.txt")

if not os.path.isfile("ExampleMod/dllmain.cpp"):
    shutil.copy(template_dllmain, f"{my_dll_mods_dir}/ExampleMod/dllmain.cpp")

# Only generates the solution if it already hasn't been already
if not os.path.isfile(solution):
    subprocess.run(f"cmake -S . -B {output_dir}")

# Building solution
subprocess.run(f"{ms_build_path} {solution} /p:Configuration={build_type} /p:Platform=x64")

# Copying the newly built files over
shutil.copy(new_example_mod_dll, old_example_mod_dll)
shutil.copy(new_ue4ss_dll, old_ue4ss_dll)
shutil.copy(new_ue4ss_xinput_dll, old_ue4ss_xinput_dll)

# Copying over deault "Mods" tree, only when it doesn't already exist
if not os.path.isdir(old_mods_dir):
    shutil.copytree(new_mods_dir, old_mods_dir)

# Copying over ue4ss ini, only when it doesn't already exist
if not os.path.isfile(old_ue4ss_ini):
    shutil.copy(new_ue4ss_ini, old_ue4ss_ini)

# Copying over pdb debugging file
if build_type == "Debug":
    shutil.copy(new_ue4ss_pdb, old_ue4ss_pdb)
    shutil.copy(new_example_mod_pdb, old_example_mod_pdb)
    shutil.copy(new_ue4ss_xinput_pdb, old_ue4ss_xinput_pdb)

# This is for removing pdbs from if we previously ran this script in "Debug", but are now in "Release"
if build_type == "Release":
    if os.path.isfile(old_example_mod_pdb):
        os.unlink(old_example_mod_pdb)

    if os.path.isfile(old_ue4ss_pdb):
        os.unlink(old_ue4ss_pdb)

    if os.path.isfile(old_ue4ss_xinput_pdb):
        os.unlink(old_ue4ss_xinput_pdb)

# Uncomment this if you want to delete output folders after building
# if os.path.isdir("Output"):
#     shutil.rmtree("Output")

# if os.path.isdir(f"{build_type}Output"):
#     shutil.rmtree(f"{build_type}Output")

if game_run_type == "win_64_exe":
    subprocess.Popen(game_exe)

if game_run_type == "steam":
    os.system(f"start steam steam://rungameid/{steam_game_app_id}")

sys.exit()
