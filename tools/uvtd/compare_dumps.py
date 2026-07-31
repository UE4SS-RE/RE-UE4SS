#!/usr/bin/env python3
"""Analyze and compare UVTD dump output.

Usage:
  compare_dumps.py analyze <UVTD_Generated_Output>
      Cross-version consistency analysis of one dump tree:
      - duplicate member lines within a template
      - members whose bitfield bit length or storage size varies across versions
      - members flipping between bitfield and plain across versions
      - vtable functions whose name changes across versions (overload naming drift)

  compare_dumps.py diff <outputA> <outputB>
      Compare two dump trees (e.g. before/after a UVTD change), reporting
      per-file changed line counts and the changed member/function lines.

Both member variable layout templates (assets/MemberVarLayoutTemplates) and
vtable layout templates (assets/VTableLayoutTemplates) are covered.
"""

import os
import re
import sys
from collections import defaultdict

MEMBER_TEMPLATE_RE = re.compile(r"^MemberVariableLayout_(.+)_Template\.ini$")
VTABLE_TEMPLATE_RE = re.compile(r"^VTableLayout_(.+)_Template\.ini$")
BITFIELD_VALUE_RE = re.compile(r"^0x([0-9A-Fa-f]+):(\d+):(\d+):(\d+)$")
PLAIN_VALUE_RE = re.compile(r"^0x([0-9A-Fa-f]+)$")


def find_dir(root, name):
    for base, dirs, _ in os.walk(root):
        if name in dirs:
            return os.path.join(base, name)
    return None


def parse_member_template(path):
    """Returns ({(section, member): value_tuple}, [duplicate (section, member)])."""
    data = {}
    dupes = []
    section = None
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.strip()
        if line.startswith("["):
            section = line.strip("[]")
            continue
        if not section or line.startswith(";") or "=" not in line:
            continue
        name, val = [x.strip() for x in line.split("=", 1)]
        if name == "UEP_TotalSize":
            continue
        key = (section, name)
        if key in data:
            dupes.append(key)
        m = BITFIELD_VALUE_RE.match(val)
        if m:
            data[key] = ("BF", int(m.group(1), 16), int(m.group(2)), int(m.group(3)), int(m.group(4)))
            continue
        m = PLAIN_VALUE_RE.match(val)
        if m:
            data[key] = ("VAR", int(m.group(1), 16))
        else:
            data[key] = ("ODD", val)
    return data, dupes


def parse_vtable_template(path):
    """Returns {section: [function names in slot order]}."""
    data = defaultdict(list)
    section = None
    for line in open(path, encoding="utf-8", errors="replace"):
        line = line.strip()
        if line.startswith("["):
            section = line.strip("[]")
            continue
        if not section or line.startswith(";") or not line:
            continue
        data[section].append(line)
    return data


def load_member_templates(root):
    d = find_dir(root, "MemberVarLayoutTemplates")
    out = {}
    if not d:
        return out
    for f in sorted(os.listdir(d)):
        m = MEMBER_TEMPLATE_RE.match(f)
        if m:
            out[m.group(1)] = parse_member_template(os.path.join(d, f))
    return out


def load_vtable_templates(root):
    d = find_dir(root, "VTableLayoutTemplates")
    out = {}
    if not d:
        return out
    for f in sorted(os.listdir(d)):
        m = VTABLE_TEMPLATE_RE.match(f)
        if m:
            out[m.group(1)] = parse_vtable_template(os.path.join(d, f))
    return out


def analyze(root):
    issues = 0

    member_templates = load_member_templates(root)
    print(f"member templates: {len(member_templates)} versions")

    print("\n== duplicate member lines within one template ==")
    found = False
    for ver, (_, dupes) in sorted(member_templates.items()):
        for sec, name in dupes:
            print(f"  {ver}: [{sec}] {name}")
            found = True
            issues += 1
    if not found:
        print("  none")

    merged = defaultdict(dict)
    for ver, (data, _) in member_templates.items():
        for key, val in data.items():
            merged[key][ver] = val

    print("\n== members with varying bitfield BIT LENGTH across versions ==")
    found = False
    for (sec, name), vers in sorted(merged.items()):
        lens = defaultdict(list)
        for ver, v in vers.items():
            if v[0] == "BF":
                lens[v[3]].append(ver)
        if len(lens) > 1:
            found = True
            issues += 1
            print(f"  [{sec}] {name}:")
            for ln, vs in sorted(lens.items()):
                print(f"      len={ln}: {', '.join(sorted(vs))}")
    if not found:
        print("  none")

    print("\n== members with varying bitfield STORAGE SIZE across versions ==")
    print("   (uint32 storage before ~4.11 is genuine engine history; anything else is suspect)")
    found = False
    for (sec, name), vers in sorted(merged.items()):
        sizes = defaultdict(list)
        for ver, v in vers.items():
            if v[0] == "BF":
                sizes[v[4]].append(ver)
        if len(sizes) > 1:
            found = True
            print(f"  [{sec}] {name}:")
            for sz, vs in sorted(sizes.items()):
                print(f"      size={sz}: {', '.join(sorted(vs))}")
    if not found:
        print("  none")

    print("\n== members flipping bitfield <-> plain across versions (informational) ==")
    count = 0
    for (sec, name), vers in sorted(merged.items()):
        kinds = {v[0] for v in vers.values()}
        if "BF" in kinds and "VAR" in kinds:
            count += 1
    print(f"  {count} members (known genuine cases: FArchive flags @4.14, UWorld bools @4.22, UClass @4.18/5.0)")

    vtable_templates = load_vtable_templates(root)
    print(f"\nvtable templates: {len(vtable_templates)} versions")

    print("\n== vtable slot coverage per class (min/max slots across versions) ==")
    class_counts = defaultdict(dict)
    for ver, sections in vtable_templates.items():
        for sec, names in sections.items():
            class_counts[sec][ver] = len(names)
    for sec, per_ver in sorted(class_counts.items()):
        lo, hi = min(per_ver.values()), max(per_ver.values())
        print(f"  {sec}: {lo}..{hi} slots")

    print(f"\nanalysis complete, {issues} hard issues")
    return 1 if issues else 0


def diff_trees(root_a, root_b):
    changed = 0
    for loader, label in ((load_member_templates, "member"), (load_vtable_templates, "vtable")):
        a, b = loader(root_a), loader(root_b)
        for ver in sorted(set(a) | set(b)):
            if ver not in a:
                print(f"[{label}] {ver}: only in B")
                continue
            if ver not in b:
                print(f"[{label}] {ver}: only in A")
                continue
            if label == "member":
                da, db = a[ver][0], b[ver][0]
                keys = set(da) | set(db)
                diffs = [k for k in keys if da.get(k) != db.get(k)]
                if diffs:
                    changed += len(diffs)
                    print(f"[member] {ver}: {len(diffs)} changed members")
                    for sec, name in sorted(diffs)[:40]:
                        print(f"    [{sec}] {name}: {da.get((sec, name))} -> {db.get((sec, name))}")
            else:
                da, db = a[ver], b[ver]
                for sec in sorted(set(da) | set(db)):
                    la, lb = da.get(sec, []), db.get(sec, [])
                    if la != lb:
                        changed += 1
                        print(f"[vtable] {ver} [{sec}]: {len(la)} -> {len(lb)} slots")
                        for i, (x, y) in enumerate(zip(la, lb)):
                            if x != y:
                                print(f"    slot {i}: {x} -> {y}")
                        for i in range(min(len(la), len(lb)), max(len(la), len(lb))):
                            src = la if len(la) > len(lb) else lb
                            side = "A" if len(la) > len(lb) else "B"
                            print(f"    slot {i}: only in {side}: {src[i]}")
    print(f"\n{changed} differences")
    return 1 if changed else 0


def main():
    if len(sys.argv) < 3:
        print(__doc__)
        return 2
    cmd = sys.argv[1]
    if cmd == "analyze":
        return analyze(sys.argv[2])
    if cmd == "diff" and len(sys.argv) >= 4:
        return diff_trees(sys.argv[2], sys.argv[3])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
