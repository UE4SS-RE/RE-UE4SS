# Generates a proxy export file for UE4SS-cppsdk.dll
# This is needed to support both ue4ss.dll and xinput1_3.dll as loaders
# cppsdk gets compiled to two variants, which are UE4SS-cppsdk.dll (ue4ss) and UE4SS-cppsdk_xinput.dll(xinput1_3)
# The sdk should be copied into the Mods directory and renamed to UE4SS-cppsdk.dll
import os
import sys
import subprocess
from pathlib import Path

input_lib = sys.argv[1]
input_lib_name = Path(input_lib).name.split(".")[0]
output_def = sys.argv[2]

result = subprocess.run(["dumpbin", "/exports", input_lib], stdout=subprocess.PIPE)

lines = result.stdout.decode('utf-8').splitlines()

start_line = 0
end_line = 0

def renumerate(sequence, start=None):
    if start is None:
        start = len(sequence) - 1
    n = start
    for elem in sequence[::-1]:
        yield n, elem
        n -= 1

for i, line in enumerate(lines):
    if "ordinal" in line and "name" in line:
        start_line = i + 2 # + 2 because next line is empty
        break

for i, line in renumerate(lines):
    if "Summary" in line:
        end_line = i - 2 # - 2 because previous line is empty
        break

lines = map(lambda x: x.strip(), lines[start_line:end_line])
lines = map(lambda x: x.split(" ")[0], lines)
lines = filter(lambda x: not x.isnumeric(), lines) # filtering ordinals

output = open(output_def, "w")

output.write("LIBRARY UE4SS-cppsdk.dll\n")
output.write("EXPORTS\n")

for line in lines:
    output.write("    " + line + "=" + input_lib_name + "." + line + "\n")

output.close()