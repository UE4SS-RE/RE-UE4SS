#!/usr/bin/env python3

import re
import os
import shutil
import subprocess
import argparse
from datetime import datetime
import sys
import json

class ReleaseHandler:
    def __init__(self, is_dev_release, is_experimental, release_output='release'):
        self.is_dev_release = is_dev_release
        self.is_experimental = is_experimental
        self.release_output = release_output
        self.staging_dir = os.path.join(self.release_output, 'StagingDev') if self.is_dev_release else os.path.join(self.release_output, 'StagingRelease')
        self.ue4ss_dir = os.path.join(self.staging_dir, 'ue4ss')

        # TODO: Move all these hardcoded values into a release config file or similar to pass into the script
        # List of CPP Mods with flags indicating if they need a config folder and if they should be included in release builds
        self.cpp_mods = {
            'KismetDebuggerMod': {'create_config': True, 'include_in_release': False},
        }

        # Lua mods to exclude from the non-dev/release version of the zip
        # And remove their entries from mods files
        self.lua_mods_to_exclude_from_release = [
            'ActorDumperMod',
            'jsbLuaProfilerMod',
        ]

        # Files in root/assets to exclude from the non-dev/release version of the zip 
        self.files_to_exclude_from_release = [
            'Mods/shared/Types.lua',
            'Mods/shared/jsbProfiler',
            'UE4SS_Signatures',
            'VTableLayoutTemplates',
            'MemberVarLayoutTemplates',
            'CustomGameConfigs',
            'MapGenBP',
            'Changelog.md'
        ]

        # Settings to change in the release. The default settings in assets/UE4SS-settings.ini are for dev
        self.settings_to_modify_in_release = {
            'GuiConsoleVisible': 0,
            'GuiConsoleEnabled': 0,
            'ConsoleEnabled': 0,
            'EnableHotReloadSystem': 0,
            'MaxMemoryUsageDuringAssetLoading': 80,
            'bUseUObjectArrayCache': "false",
        }

    def make_staging_dirs(self):
        shutil.copytree('assets', self.ue4ss_dir)
        shutil.copy('LICENSE', os.path.join(self.ue4ss_dir, 'LICENSE'))

        if not self.is_dev_release:
            for file in self.files_to_exclude_from_release:
                path = os.path.join(self.ue4ss_dir, file)
                if os.path.exists(path):
                    if os.path.isfile(path):
                        os.remove(path)
                    elif os.path.isdir(path):
                        shutil.rmtree(path)
            self.modify_settings(self.settings_to_modify_in_release)

            # Remove lua mods from the Mods directory
            for mod in self.lua_mods_to_exclude_from_release:
                mod_path = os.path.join(self.ue4ss_dir, 'Mods', mod)
                if os.path.exists(mod_path):
                    if os.path.isfile(mod_path):
                        os.remove(mod_path)
                    elif os.path.isdir(mod_path):
                        shutil.rmtree(mod_path)
        else:
            # only include README file in dev releases
            shutil.copy('README.md', os.path.join(self.ue4ss_dir, 'README.md'))

        ue4ss_dll_path, ue4ss_pdb_path, dwmapi_dll_path, cpp_mods_paths = self.scan_directories()

        self.copy_cpp_mods(cpp_mods_paths)
        self.modify_mods_txt() # can only run this after copy mods just in case we are missing mod dlls
        self.modify_mods_json()
        self.copy_executables(dwmapi_dll_path, ue4ss_dll_path, ue4ss_pdb_path)
        self.copy_docs()

        # needs to be run inside staging as we don't want to modify the original
        # only run this if we are not doing an experimental release, as we don't want the date to be set in experimental releases
        if not self.is_experimental: self.modify_changelog() 

    def modify_settings(self, settings_to_modify):
        config_path = os.path.join(self.ue4ss_dir, 'UE4SS-settings.ini')
        with open(config_path, mode='r', encoding='utf-8-sig') as file:
            content = file.read()

        for key, value in settings_to_modify.items():
            pattern = rf'(^{key}\s*=).*?$'
            content = re.sub(pattern, rf'\1 {value}', content, flags=re.MULTILINE)

        with open(config_path, mode='w', encoding='utf-8-sig') as file:
            file.write(content)

    def scan_directories(self):
        ue4ss_dll_path = ''
        ue4ss_pdb_path = ''
        dwmapi_dll_path = ''
        cpp_mods_paths = {mod: '' for mod in self.cpp_mods if self.is_dev_release or self.cpp_mods[mod]['include_in_release']}
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

    def copy_cpp_mods(self, cpp_mods_paths):
        for mod_name, dll_path in cpp_mods_paths.items():
            if dll_path:
                mod_dir = os.path.join(self.ue4ss_dir, 'Mods', mod_name, 'dlls')
                os.makedirs(mod_dir, exist_ok=True)
                shutil.copy(dll_path, os.path.join(mod_dir, 'main.dll'))
                if self.is_dev_release: 
                    pdb_path = dll_path.replace('.dll', '.pdb')
                    if os.path.exists(pdb_path):
                        shutil.copy(pdb_path, os.path.join(mod_dir, 'main.pdb'))
            else:
                print(f'Error: {mod_name}.dll not found, build has failed.')
                sys.exit(1)

        for mod_name, mod_info in self.cpp_mods.items():
            if self.is_dev_release or mod_info['include_in_release']:
                if mod_info['create_config']:
                    os.makedirs(os.path.join(self.ue4ss_dir, 'Mods', mod_name, 'config'), exist_ok=True)

    def modify_mods_txt(self):
        mods_to_remove_from_release = self.lua_mods_to_exclude_from_release.copy()
        for mod_name, mod_info in self.cpp_mods.items():
            if not mod_info['include_in_release']:
                mods_to_remove_from_release.append(mod_name)

        mods_path = os.path.join(self.ue4ss_dir, 'Mods', 'mods.txt')
        with open(mods_path, mode='r', encoding='utf-8-sig') as file:
            content = file.readlines()

        if self.cpp_mods:
            content = [f'{mod} : 1\n' for mod in self.cpp_mods] + content

        if not self.is_dev_release:
            content = [line for line in content if not any(mod in line for mod in mods_to_remove_from_release)]

        with open(mods_path, mode='w', encoding='utf-8-sig') as file:
            file.writelines(content)

    def modify_mods_json(self):
        mods_path = os.path.join(self.ue4ss_dir, 'Mods', 'mods.json')
        with open(mods_path, mode='r', encoding='utf-8-sig') as file:
            content = json.load(file)

        if self.cpp_mods:
            for mod_name, mod_info in self.cpp_mods.items():
                if mod_name not in [mod['mod_name'] for mod in content]:
                    if not self.is_dev_release:
                        content.append({'mod_name': mod_name, 'mod_enabled': mod_info['include_in_release']})
                    else:
                        content.append({'mod_name': mod_name, 'mod_enabled': True})

        if not self.is_dev_release:
            content = [
                mod for mod in content
                if mod['mod_name'] not in self.lua_mods_to_exclude_from_release
                and self.cpp_mods.get(mod['mod_name'], {}).get('include_in_release', True)
            ]

        with open(mods_path, mode='w', encoding='utf-8-sig') as file:
            json.dump(content, file, indent=4)

    def copy_executables(self, dwmapi_dll_path, ue4ss_dll_path, ue4ss_pdb_path):
        shutil.copy(dwmapi_dll_path, self.staging_dir)
        shutil.copy(ue4ss_dll_path, self.ue4ss_dir)
        if self.is_dev_release: shutil.copy(ue4ss_pdb_path, self.ue4ss_dir)

    def copy_docs(self):
        if self.is_dev_release and os.path.exists('docs'):
            shutil.copytree('docs', os.path.join(self.ue4ss_dir, 'Docs'))

    def modify_changelog(self):
        changelog_path = os.path.join(self.ue4ss_dir, 'Changelog.md')
        if os.path.exists(changelog_path): ready_changelog_for_release(changelog_path)

    def package_release(self):
        try:
            version = subprocess.check_output(['git', 'describe', '--tags', '--exclude', '*experimental*']).decode('utf-8').strip()
        except subprocess.CalledProcessError:
            print('Error: git describe failed. Make sure the release has been tagged.')
            sys.exit(1)
        main_zip_name = f'zDEV-UE4SS_{version}' if self.is_dev_release else f'UE4SS_{version}'
        output = os.path.join(self.release_output, main_zip_name)
        shutil.make_archive(output, 'zip', self.staging_dir)
        print(f'Created package {output}.zip')

    def cleanup(self):
        shutil.rmtree(self.staging_dir)

class Packager:
    """
    "Release" refers to the zip that is distributed to the end users. It should be in the format UE4SS_<version>.zip and contains only the necessary files.
    "Dev" refers to the zip that is used for development. It should be in the format zDEV-UE4SS_<version>.zip and contains all the files, pdbs and docs.
    """

    def __init__(self, args):
        self.args = args
        self.release_output = 'release'
        self.setup()

    def setup(self):
        if os.path.exists(self.release_output): shutil.rmtree(self.release_output)
        os.mkdir(self.release_output)

    def run(self):
        dev_handler = ReleaseHandler(is_dev_release=True, is_experimental=self.args.e, release_output=self.release_output)
        release_handler = ReleaseHandler(is_dev_release=False, is_experimental=self.args.e, release_output=self.release_output)

        dev_handler.make_staging_dirs()
        dev_handler.package_release()
        dev_handler.cleanup()

        release_handler.make_staging_dirs()
        release_handler.package_release()
        release_handler.cleanup()

        shutil.make_archive(os.path.join(self.release_output, 'zCustomGameConfigs'), 'zip', 'assets/CustomGameConfigs')
        shutil.make_archive(os.path.join(self.release_output, 'zMapGenBP'), 'zip', 'assets/MapGenBP')

        changelog = parse_changelog(self.args.changelog_path)
        with open(os.path.join(self.release_output, 'release_notes.md'), 'w') as file:
            file.write(changelog[0]['notes'])

        print('Done')

def ready_changelog_for_release(changelog_path):
    with open(changelog_path, 'r') as file:
        lines = file.readlines()
    version = lines[0].strip()
    if lines[2] != 'TBD\n':
        raise Exception('date is not "TBD"')
    lines[2] = datetime.today().strftime('%Y-%m-%d') + '\n'
    with open(changelog_path, 'w') as file:
        file.writelines(lines)
    return version

def parse_changelog(changelog_path):
    with open(changelog_path, 'r') as file:
        lines = file.readlines()
        delimeters = [index - 1 for index, value in enumerate(lines) if value == '==============\n']
        delimeters.append(len(lines) + 1)
        return [{
            'tag': lines[index[0]].strip(),
            'date': lines[index[0] + 2].strip(),
            'notes': ''.join(lines[index[0] + 3:index[1]]).strip(),
        } for index in zip(delimeters, delimeters[1:])]

def package(args):
    packager = Packager(args)
    packager.run()

def release_commit(args):
    version = ready_changelog_for_release(args.changelog_path)
    message = f'Release {version}'
    subprocess.run(['git', 'add', args.changelog_path], check=True)
    if args.username:
        subprocess.run(['git', '-c', f'user.name="{args.username}"', '-c', f'user.email="{args.username}@users.noreply.github.com"', 'commit', '-m', message], check=True)
    else:
        subprocess.run(['git', 'commit', '-m', message], check=True)
    subprocess.run(['git', 'tag', version], check=True)

    # Outputs to GitHub env if present
    def github_output(name, value):
        if 'GITHUB_OUTPUT' in os.environ:
            with open(os.environ['GITHUB_OUTPUT'], 'a') as env:
                env.write(f'{name}={value}\n')

    github_output('release_tag', version)

if __name__ == "__main__":
    # Change dir to repo root
    os.chdir(os.path.join(os.path.dirname(__file__), '..', '..'))

    parser = argparse.ArgumentParser()
    parser.add_argument('--changelog_path', default='assets/Changelog.md', required=False)

    subparsers = parser.add_subparsers(dest='command', required=True)

    package_parser = subparsers.add_parser('package')
    package_parser.add_argument('-e', action='store_true')

    release_commit_parser = subparsers.add_parser('release_commit')
    release_commit_parser.add_argument('username', nargs='?')

    args = parser.parse_args()
    commands = {f.__name__: f for f in [
        package,
        release_commit,
    ]}
    commands[args.command](args)

"""
How to run this script:
1. Running the 'package' command:
    Usage: python release.py package

    --changelog_path : Optional argument to specify the path to the changelog file. Default is 'assets/Changelog.md'
    -e : Argument used when running the script for experimental release

    Examples:
    - python release.py package 
    - python release.py package -e --changelog_path custom/Changelog.md

2. Running the 'release_commit' command:
    Usage: python release.py release_commit [username]

    username : Optional argument to specify a github username
    --changelog_path : Optional argument to specify the path to the changelog file. Default is 'assets/Changelog.md'

    Examples:
    - python release.py release_commit
    - python release.py release_commit ${{ github.actor }}
    - python release.py release_commit ${{ github.actor }} custom/Changelog.md
"""

"""
To locally debug this script in vscode, make launch.json in .vscode and add the following configurations:

{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "Python: Debug Release Script",
            "type": "debugpy",
            "request": "launch",
            "program": "${workspaceFolder}/tools/buildscripts/release.py",
            "console": "integratedTerminal",
            "args": [
                "package",
                "-e"        
            ],
            "justMyCode": true
        }
    ]
}
"""
