import os
import sys
import subprocess


# build_type = "Debug"
build_type = "Release"


script_dir = os.path.dirname(os.path.abspath(__file__))
os.chdir(script_dir)
os.chdir("../../../..")


subprocess.run(f"cmake -S . -B {build_type}Output")


sys.exit()
