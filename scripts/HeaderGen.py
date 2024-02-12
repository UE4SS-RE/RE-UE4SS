#!/usr/bin/env python3
# generate header file for C++, using DRAWF information from .debug files

import os
import sys

DEFAULT_FILENAME = "./UnrealGame-Linux-Shipping.debug"
DEFAULT_SEARCH_DIR = "../deps/first/Unreal/generated_include"
DEFAULT_GENERATED_DIR = "../generated"

FUNCTION_TEMPLATE = """if (auto it = {ClassName}::VTableLayoutMap.find(STR("{FunctionName}")); it == {ClassName}::VTableLayoutMap.end())
{{
    {ClassName}::VTableLayoutMap.emplace(STR("{FunctionName}"), 0x{FunctionOffset:02x});
}}
"""

MEMBER_TEMPLATE = """if (auto it = {ClassName}::MemberOffsets.find(STR("{MemberName}")); it == {ClassName}::MemberOffsets.end())
{{
    UWorld::MemberOffsets.emplace(STR("{MemberName}"), 0x{MemberOffset:02x});
}}
"""

class CMember:
    def __init__(self, name, offset):
        self.name = name
        self.offset = offset

    def GenerateMember(self, className, count = None):
        if count and count != 0:
            return MEMBER_TEMPLATE.format(ClassName = className, MemberName =  f"{self.name}_{count}", MemberOffset = self.offset)
        else:
            return MEMBER_TEMPLATE.format(ClassName = className, MemberName = self.name, MemberOffset = self.offset)
    
class CVFunc:
    def __init__(self, mangledname, name, slotoffset):
        self.mangledname = mangledname
        self.name = name
        self.slotoffset = slotoffset

    def GenerateVFunc(self, className, count = None):
        if count and count != 0:
            return FUNCTION_TEMPLATE.format(ClassName = className, FunctionName = f"{self.name}_{count}", FunctionOffset = self.slotoffset)
        else:
            return FUNCTION_TEMPLATE.format(ClassName = className, FunctionName = self.name, FunctionOffset = self.slotoffset)

class CClass:
    def __init__(self, name):
        self.name = name
        self.members = {}
        self.vfuncs = {}

    def AddMember(self, member, offset):
        if member not in self.members:
            self.members[member] = CMember(member, offset)
    
    def AddVFunc(self, mangledname, vfunc, offset):
        if mangledname not in self.vfuncs:
            self.vfuncs[mangledname] = CVFunc(mangledname, vfunc, offset)

    def GenerateMember(self):
        vals = list(self.members.values())
        vals.sort(key = lambda x: x.offset)
        return "\n".join([member.GenerateMember(self.name) for member in vals])

    def GenerateVTable(self):
        generatedName = {}
        results = []
        vals = list(self.vfuncs.values())
        vals.sort(key = lambda x: x.slotoffset)
        for vfunc in vals:
            if vfunc.name in generatedName:
                generatedName[vfunc.name] += 1
            else:
                generatedName[vfunc.name] = 0
            results.append(vfunc.GenerateVFunc(self.name, generatedName[vfunc.name]))
        return "\n".join(results)

class Database:
    def __init__(self):
        self.classes = {}

    def GetClass(self, name):
        if name not in self.classes:
            self.classes[name] = CClass(name)
        return self.classes[name]

    def GenerateMember(self):
        return { name: cls.GenerateMember() for name, cls in self.classes.items() }
        
    def GenerateVTable(self):
        return { name: cls.GenerateVTable() for name, cls in self.classes.items() }

def GetClassNames(search_dir = DEFAULT_SEARCH_DIR):
    # glob all file with pattern "MemberVariableLayout_HeaderWrapper_{ClassName}.hpp"
    # return list of class names
    """
    import glob, re
    return [re.search(r"MemberVariableLayout_HeaderWrapper_(.*).hpp", file).group(1) for file in glob.glob(f"{search_dir}/MemberVariableLayout_HeaderWrapper_*.hpp")]
    """
    return ['UGameViewportClient',
            'UEnum',
            'UConsole',
            'UDataTable',
            'FClassProperty',
            'FByteProperty',
            'FDelegateProperty',
            'UFunction',
            'FEnumProperty',
            'FSetProperty',
            'FInterfaceProperty',
            'FFieldPathProperty',
            'AGameMode',
            'UObjectBase',
            'FObjectPropertyBase',
            'AActor',
            'FBoolProperty',
            'UScriptStruct__ICppStructOps',
            'FMulticastDelegateProperty',
            'UPlayer',
            'FOutputDevice',
            'UField',
            'FMapProperty',
            'FArchive',
            'FSoftClassProperty',
            'FStructProperty',
            'ULocalPlayer',
            'UWorld',
            'AGameModeBase',
            'FConsoleVariableBase',
            'FConsoleCommandBase',
            'FArchiveState',
            'UScriptStruct',
            'FConsoleManager',
            'UStruct',
            'FProperty',
            'FArrayProperty',
            'UClass',
            'FField']

from elftools.dwarf.die import DIE
from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection

def ScanCU(database, classes, cu):
    for ch in cu.get_top_DIE().iter_children():
        if ch.tag == 'DW_TAG_class_type':
            if ch.attributes.get('DW_AT_name'):
                name = ch.attributes['DW_AT_name'].value.decode('utf-8')
                if name in classes:
                    print(name)
                    klass = database.GetClass(name)
                    for m in ch.iter_children():
                        name_m = m.attributes.get('DW_AT_name')
                        if not name_m:
                            continue
                        name_m = name_m.value.decode('utf-8')
                        if m.tag == 'DW_TAG_member': 
                            if m.attributes.get('DW_AT_data_member_location'):
                                print(f"  member: {name_m} @ {m.attributes['DW_AT_data_member_location'].value}")
                                klass.AddMember(name_m, m.attributes['DW_AT_data_member_location'].value)
                        if m.tag == "DW_TAG_subprogram":
                            # check if DW_AT_vtable_elem_location exists
                            if m.attributes.get('DW_AT_vtable_elem_location'):
                                # is a virtual function
                                link_name = m.attributes.get('DW_AT_linkage_name')
                                if link_name:
                                    link_name = link_name.value.decode('utf-8')
                                else:
                                    link_name = name_m
                                
                                print(f"  vfunc: {name_m} aka {link_name} @ {m.attributes['DW_AT_vtable_elem_location'].value[1] * 8}")
                                klass.AddVFunc(link_name, name_m, m.attributes['DW_AT_vtable_elem_location'].value[1] * 8)
                    print()

def ProcessFile(filename = DEFAULT_FILENAME, target_dir = DEFAULT_GENERATED_DIR, target_class = GetClassNames()):
    with open(filename, 'rb') as f:
        elffile = ELFFile(f)
        dwarfinfo = elffile.get_dwarf_info()
    
    # if database_.pkl exists, load it and resume from last progress
    started = True
    if os.path.exists("database_.pkl"):
        with open("database_.pkl", "rb") as f:
            import pickle
            database = pickle.load(f)
            progress = pickle.load(f)
            started = False
    else:
        database = Database()

    for cu in dwarfinfo.iter_CUs():
        if cu.get_top_DIE().attributes.get('DW_AT_name').value.decode('utf-8') == progress:
            started = True
            continue
        if not started:
            continue

        ScanCU(database, target_class, cu)
        # pickle dump
        with open("database_.pkl", "wb") as f:
            import pickle
            pickle.dump(database, f)
            pickle.dump(cu.get_top_DIE().attributes['DW_AT_name'].value.decode('utf-8'), f) # dump progress
        print(f"{cu.get_top_DIE().attributes['DW_AT_name'].value.decode('utf-8')} done...")

    # generate header files
    path = DEFAULT_GENERATED_DIR
    VFunc = "Linux_5_11_VTableOffsets_{ClassName}_FunctionBody.cpp"
    Member = "Linux_5_11_MemberVariableLayout_DefaultSetter_{ClassName}.cpp"
    for c in database.classes.values():
        with open(path + VFunc.format(ClassName=c.name), "w") as f:
            f.write(c.GenerateVTable())
        with open(path + Member.format(ClassName=c.name), "w") as f:
            f.write(c.GenerateMember())

def resolve_path(path):
    return os.path.abspath(os.path.expanduser(path))

if __name__ == "__main__":
    # get optional arguments
    if len(sys.argv) > 1:
        filename = sys.argv[1]
    else:
        filename = DEFAULT_FILENAME
    if len(sys.argv) > 2:
        target_dir = sys.argv[2]
    else:
        target_dir = "generated"
    if len(sys.argv) > 3:
        target_class = sys.argv[3:]
    else:
        target_class = GetClassNames()

    # check validity of arguments
    if filename == '-h' or filename == '--help':
        print(f"Usage: {sys.argv[0]} [filename] [target_dir] [target_class...]")
        print(f"    Default filename: {resolve_path(DEFAULT_FILENAME)}")
        print(f"    Default target_dir: {resolve_path(DEFAULT_GENERATED_DIR)}")
        print(f"    Default target_class: {' '.join(GetClassNames())}")
        sys.exit(0)

    if not os.path.exists(filename):
        print(f"File {filename} not found")
        sys.exit(1)

    if not os.path.exists(target_dir):
        print(f"Directory {target_dir} not found")
        sys.exit(1)
    
    rocessFile(filename, target_dir, target_class)
