r"""Embed the schema catalog dumped by FF7MMAMod into mma_mod_editor.html.

Usage: python embed_schemas.py [path\to\schemas.json]
Default schemas.json location is the FF7 Rebirth ue4ss/MMADumps folder.
"""
import json
import os
import re
import sys

DEFAULT_SCHEMAS = r"F:\SteamLibrary\steamapps\common\FINAL FANTASY VII REBIRTH\End\Binaries\Win64\ue4ss\MMADumps\schemas.json"

def main():
    schemas_path = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SCHEMAS
    html_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "mma_mod_editor.html")

    with open(schemas_path, encoding="utf-8") as f:
        catalog = json.load(f)
    compact = json.dumps(catalog, separators=(",", ":"), ensure_ascii=True)

    with open(html_path, encoding="utf-8") as f:
        html = f.read()

    pattern = re.compile(r"(// SCHEMAS_START.*?\n)const BUILTIN_SCHEMAS = .*?;\n(// SCHEMAS_END)", re.S)
    replaced, count = pattern.subn(lambda m: m.group(1) + "const BUILTIN_SCHEMAS = " + compact + ";\n" + m.group(2), html)
    if count != 1:
        print("ERROR: SCHEMAS_START/END markers not found")
        return 1

    with open(html_path, "w", encoding="utf-8") as f:
        f.write(replaced)
    print("embedded %d table schemas (%.1f KB) into %s" % (len(catalog), len(compact) / 1024, html_path))
    return 0

if __name__ == "__main__":
    sys.exit(main())
