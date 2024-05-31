#!/usr/bin/env python3

import re
import os
import shutil
import subprocess
import argparse
from datetime import datetime

class Packager:
    def __init__(self):
        self.release_output = 'release'
        self.staging_dev = os.path.join(self.release_output, 'StagingDev')
        self.staging_release = os.path.join(self.release_output, 'StagingRelease')

        shutil.rmtree(self.release_output, ignore_errors=True)
        os.mkdir(self.release_output)

        # List of CPP Mods with flags indicating if they need a config folder and if they should be included in release builds
        self.cpp_mods = {
            'KismetDebuggerMod': {'create_config': True, 'include_in_release': False},
        }

        # Disable any dev-only mods
        self.mods_to_disable_in_release = {
            'LineTraceMod': 0,
        }

        # Files to exclude from the non-dev/release version of the zip 
        self.files_to_exclude_from_release = [
            'Mods/shared/Types.lua',
            'UE4SS_Signatures',
            'VTableLayoutTemplates',
            'MemberVarLayoutTemplates',
            'CustomGameConfigs',
            'MapGenBP',
        ]

        # Settings to change in the release. The default settings in assets/UE4SS-settings.ini are for dev
        self.settings_to_modify_in_release = {
            'GuiConsoleVisible': 0,
            'ConsoleEnabled': 0,
            'EnableHotReloadSystem': 0,
            'MaxMemoryUsageDuringAssetLoading': 80,
            'GUIUFunctionCaller': 0,
        }

    def make_staging_dirs(self, is_dev_release: bool):
        staging_dir = self.staging_dev if is_dev_release else self.staging_release
        staging_dir = os.path.join(staging_dir, 'ue4ss')
        shutil.copytree('assets', staging_dir)
        shutil.copy('README.md', os.path.join(staging_dir, 'README.md'))

        if not is_dev_release:
            for file in self.files_to_exclude_from_release:
                path = os.path.join(staging_dir, file)
                try:
                    os.remove(path)
                except:
                    shutil.rmtree(path)
            self.modify_settings(staging_dir, self.settings_to_modify_in_release)

    def modify_settings(self, staging_dir, settings_to_modify):
        config_path = os.path.join(staging_dir, 'UE4SS-settings.ini')
        with open(config_path, mode='r', encoding='utf-8-sig') as file:
            content = file.read()

        for key, value in settings_to_modify.items():
            pattern = rf'(^{key}\s*=).*?$'
            content = re.sub(pattern, rf'\1 {value}', content, flags=re.MULTILINE)

        with open(config_path, mode='w', encoding='utf-8-sig') as file:
            file.write(content)
    
    def package_release(self, is_dev_release: bool):
        version = subprocess.check_output(['git', 'describe', '--tags']).decode('utf-8').strip()
        if is_dev_release:
            main_zip_name = f'zDEV-UE4SS_{version}'
            staging_dir = self.staging_dev
        else:
            main_zip_name = f'UE4SS_{version}'
            staging_dir = self.staging_release

        ue4ss_dll_path, ue4ss_pdb_path, dwmapi_dll_path, cpp_mods_paths = self.scan_directories(is_dev_release)
        ue4ss_dir = os.path.join(staging_dir, 'ue4ss')

        self.copy_cpp_mods(ue4ss_dir, cpp_mods_paths, is_dev_release)
        self.modify_mods_txt(ue4ss_dir, is_dev_release) # can only run this after copy mods just in case we are missing mod dlls
        self.copy_executables(staging_dir, ue4ss_dir, dwmapi_dll_path, ue4ss_dll_path, ue4ss_pdb_path, is_dev_release)
        self.copy_docs(ue4ss_dir, is_dev_release)

        output = os.path.join(self.release_output, main_zip_name)
        shutil.make_archive(output, 'zip', staging_dir)
        print(f'Created package {output}.zip')

    def scan_directories(self, is_dev_release):
        ue4ss_dll_path = ''
        ue4ss_pdb_path = ''
        dwmapi_dll_path = ''
        cpp_mods_paths = {mod: '' for mod in self.cpp_mods if is_dev_release or self.cpp_mods[mod]['include_in_release']}
        scan_start_dir = '.'
        
        for root, _, files in os.walk(scan_start_dir):
            for file in files:
                if file.lower() == "ue4ss.dll":
                    ue4ss_dll_path = os.path.join(root, file)
                if file.lower() == "ue4ss.pdb":
                    ue4ss_pdb_path = os.path.join(root, file)
                if file.lower() == "dwmapi.dll":
                    dwmapi_dll_path = os.path.join(root, file)
                for mod_name in cpp_mods_paths:
                    if file.lower() == mod_name.lower() + '.dll':
                        cpp_mods_paths[mod_name] = os.path.join(root, file)
        
        return ue4ss_dll_path, ue4ss_pdb_path, dwmapi_dll_path, cpp_mods_paths

    def copy_cpp_mods(self, staging_dir, cpp_mods_paths, is_dev_release):
        for mod_name, dll_path in cpp_mods_paths.items():
            if dll_path:
                mod_dir = os.path.join(staging_dir, 'Mods', mod_name, 'dlls')
                os.makedirs(mod_dir, exist_ok=True)
                shutil.copy(dll_path, os.path.join(mod_dir, 'main.dll'))
            else:
                print(f'Warning: {mod_name}.dll not found, skipping')
                self.cpp_mods.pop(mod_name)

        for mod_name, mod_info in self.cpp_mods.items():
            if is_dev_release or mod_info['include_in_release']:
                if mod_info['create_config']:
                    os.makedirs(os.path.join(staging_dir, 'Mods', mod_name, 'config'), exist_ok=True)

    def modify_mods_txt(self, staging_dir, is_dev_release):
        for mod_name, mod_info in self.cpp_mods.items():
            if not mod_info['include_in_release']:
                self.mods_to_disable_in_release[mod_name] = 0
        
        mods_path = os.path.join(staging_dir, 'Mods/mods.txt')
        with open(mods_path, mode='r', encoding='utf-8-sig') as file:
            content = file.read()

        if self.cpp_mods: content = '\n'.join([f'{mod} : 1' for mod in self.cpp_mods]) + '\n' + content

        if not is_dev_release:
            for key, value in self.mods_to_disable_in_release.items():
                pattern = rf'(^{key}\s*:).*?$'
                content = re.sub(pattern, rf'\1 {value}', content, flags=re.MULTILINE)

        with open(mods_path, mode='w', encoding='utf-8-sig') as file:
            file.write(content)

    def copy_executables(self, staging_dir, ue4ss_dir, dwmapi_dll_path, ue4ss_dll_path, ue4ss_pdb_path, is_dev_release):
        shutil.copy(dwmapi_dll_path, staging_dir)
        shutil.copy(ue4ss_dll_path, ue4ss_dir)
        if is_dev_release: shutil.copy(ue4ss_pdb_path, ue4ss_dir)

    def copy_docs(self, ue4ss_dir, is_dev_release):
        if is_dev_release and os.path.exists('docs'): shutil.copytree('docs', os.path.join(ue4ss_dir, 'Docs'))

    def run(self):
        self.make_staging_dirs(is_dev_release=True)
        self.make_staging_dirs(is_dev_release=False)

        self.package_release(is_dev_release=True)
        self.package_release(is_dev_release=False)

        shutil.make_archive(os.path.join(self.release_output, 'zCustomGameConfigs'), 'zip', 'assets/CustomGameConfigs')
        shutil.make_archive(os.path.join(self.release_output, 'zMapGenBP'), 'zip', 'assets/MapGenBP')

        self.cleanup()

        changelog = parse_changelog()
        with open(os.path.join(self.release_output, 'release_notes.md'), 'w') as file:
            file.write(changelog[0]['notes'])

        print('Done')

    def cleanup(self):
        shutil.rmtree(self.staging_dev, ignore_errors=True)
        shutil.rmtree(self.staging_release, ignore_errors=True)

changelog_path = 'assets/Changelog.md'

# Outputs to GitHub env if present
def github_output(name, value):
    if 'GITHUB_OUTPUT' in os.environ:
        with open(os.environ['GITHUB_OUTPUT'], 'a') as env:
            env.write(f'{name}={value}\n')

def parse_changelog():
    with open(changelog_path, 'r') as file:
        lines = file.readlines()
        delimeters = [index - 1 for index, value in enumerate(lines) if value == '==============\n']
        delimeters.append(len(lines) + 1)
        return [{
            'tag': lines[index[0]].strip(),
            'date': lines[index[0] + 2].strip(),
            'notes': ''.join(lines[index[0] + 3:index[1]]).strip(),
        } for index in zip(delimeters, delimeters[1:])]

def get_release_notes():
    changelog = parse_changelog()
    print(changelog[0]['notes'])

def release_commit(args):
    # TODO perhaps check if index is dirty to avoid clobbering anything

    with open(changelog_path, mode='r') as file:
        lines = file.readlines()
    version = lines[0].strip()
    if lines[2] != 'TBD\n':
        raise Exception('date is not "TBD"')
    lines[2] = datetime.today().strftime('%Y-%m-%d') + '\n'

    with open(changelog_path, mode='w') as file:
        file.writelines(lines)

    message = f'Release {version}'
    subprocess.run(['git', 'add', changelog_path], check=True)
    if args.username:
        subprocess.run(['git', '-c', f'user.name="{args.username}"', '-c', f'user.email="{args.username}@users.noreply.github.com"', 'commit', '-m', message], check=True)
    else:
        subprocess.run(['git', 'commit', '-m', message], check=True)
    subprocess.run(['git', 'tag', version], check=True)

    github_output('release_tag', version)

def package(args):
    packager = Packager(args)
    packager.run()

if __name__ == "__main__":
    # Change dir to repo root
    os.chdir(os.path.join(os.path.dirname(__file__), '..', '..'))
    
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest='command', required=True)
    
    package_parser = subparsers.add_parser('package')
    
    release_commit_parser = subparsers.add_parser('release_commit')
    release_commit_parser.add_argument('username', nargs='?')
    
    args = parser.parse_args()
    commands = {f.__name__: f for f in [
        get_release_notes,
        package,
        release_commit,
    ]}
    commands[args.command](args)

"""
How to run this script:

1. Running the 'package' command:
    Usage: python release.py package

2. Running the 'release_commit' command:
    Usage: python /release.py release_commit [username]
    
    username : Optional argument to specify a github username
    
    Examples:
    - python release.py release_commit
    - python release.py release_commit ${{ github.actor }}
"""