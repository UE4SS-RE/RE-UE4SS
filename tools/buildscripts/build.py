#!/usr/bin/env python3

import re
import os
import shutil
import subprocess
import argparse
from datetime import datetime

# Change dir to repo root
os.chdir(os.path.join(os.path.dirname(__file__), '..', '..'))

# Outputs to GitHub env if present
def github_output(name, value):
    if 'GITHUB_OUTPUT' in os.environ:
        with open(os.environ['GITHUB_OUTPUT'], 'a') as env:
            env.write(f'{name}={value}\n')

changelog_path = 'assets/Changelog.md'

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

def get_release_notes(args):
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
    is_experimental = args.e

    release_output = 'release'
    shutil.rmtree(release_output, ignore_errors=True)
    os.mkdir(release_output)

    staging_dev = os.path.join(release_output, 'StagingDev')
    staging_release = os.path.join(release_output, 'StagingRelease')

    def make_staging_dirs():
        # Builds a release version of /assets by copying the directory and then
        # removing and disabling dev-only settings and files
        exclude_files = [
            'Mods/shared/Types.lua',
            'UE4SS_Signatures',
            'VTableLayoutTemplates',
            'MemberVarLayoutTemplates',
            'CustomGameConfigs',
            'MapGenBP',
        ]

        settings_to_modify_in_release = {
            'GuiConsoleVisible': 0,
            'ConsoleEnabled': 0,
            'EnableHotReloadSystem': 0,
            'IgnoreEngineAndCoreUObject': 1,
            'MaxMemoryUsageDuringAssetLoading': 80,
            'GUIUFunctionCaller': 0,
        }

        change_modstxt = {
            'LineTraceMod': 0,
        }

        # Copy whole directory
        shutil.copytree('assets', staging_dev)
        shutil.copytree('assets', staging_release)

        # Include repo README
        shutil.copy('README.md', os.path.join(staging_dev, 'README.md'))
        shutil.copy('README.md', os.path.join(staging_release, 'README.md'))

        # Remove files
        for file in exclude_files:
            path = os.path.join(staging_release, file)
            try:
                os.remove(path)
            except:
                shutil.rmtree(path)

        # Change UE4SS-settings.ini
        config_path = os.path.join(staging_release, 'UE4SS-settings.ini')

        with open(config_path, mode='r', encoding='utf-8-sig') as file:
            content = file.read()

        for key, value in settings_to_modify_in_release.items():
            pattern = rf'(^{key}\s*=).*?$'
            content = re.sub(pattern, rf'\1 {value}', content, flags=re.MULTILINE)

        with open(config_path, mode='w', encoding='utf-8-sig') as file:
            file.write(content)

        # Change Mods/mods.txt
        mods_path = os.path.join(staging_release, 'Mods/mods.txt')

        with open(mods_path, mode='r', encoding='utf-8-sig') as file:
            content = file.read()

        for key, value in change_modstxt.items():
            pattern = rf'(^{key}\s*:).*?$'
            content = re.sub(pattern, rf'\1 {value}', content, flags=re.MULTILINE)

        with open(mods_path, mode='w', encoding='utf-8-sig') as file:
            file.write(content)

    def package_release(is_dev_release: bool):
        version = subprocess.check_output(['git', 'describe', '--tags']).decode('utf-8').strip()
        if is_dev_release:
            main_zip_name = f'zDEV-UE4SS_{version}'
            staging_dir = staging_dev
        else:
            main_zip_name = f'UE4SS_{version}'
            staging_dir = staging_release

        ue4ss_dll_path = ''
        ue4ss_pdb_path = ''
        dwmapi_dll_path = ''

        scan_start_dir = '.'
        if str(args.d) != 'None':
            scan_start_dir = str(args.d)

        for root, dirs, files in os.walk(scan_start_dir):
            for file in files:
                if file.lower() == "ue4ss.dll":
                    ue4ss_dll_path = os.path.join(root, file)
                if file.lower() == "ue4ss.pdb":
                    ue4ss_pdb_path = os.path.join(root, file)
                if file.lower() == "dwmapi.dll":
                    dwmapi_dll_path = os.path.join(root, file)

        # Create the ue4ss folder in staging_dir
        ue4ss_dir = os.path.join(staging_dir, 'ue4ss')
        os.makedirs(ue4ss_dir, exist_ok=True)

        # Move all files from assets folder to the ue4ss folder except dwmapi.dll
        for root, _, files in os.walk('assets'):
            for file in files:
                if file.lower() != 'dwmapi.dll':
                    src_path = os.path.join(root, file)
                    dst_path = os.path.join(ue4ss_dir, os.path.relpath(src_path, 'assets'))
                    os.makedirs(os.path.dirname(dst_path), exist_ok=True)
                    shutil.copy(src_path, dst_path)

        # Main dll and pdb
        shutil.copy(ue4ss_dll_path, ue4ss_dir)

        # Proxy
        shutil.copy(dwmapi_dll_path, staging_dir)

        if is_dev_release:
            shutil.copy(ue4ss_pdb_path, ue4ss_dir)
            if os.path.exists(os.path.join(scan_start_dir, 'docs')):
                shutil.copytree('docs', os.path.join(ue4ss_dir, 'Docs'))

        # Move remaining files to the ue4ss dir
        dont_move = ['dwmapi.dll', 'docs', 'ue4ss']
        for file in os.listdir(staging_dir):
            if file.lower() not in dont_move:
                shutil.move(os.path.join(staging_dir, file), os.path.join(ue4ss_dir, file))

        output = os.path.join(release_output, main_zip_name)
        shutil.make_archive(output, 'zip', staging_dir)
        print(f'Created package {output}.zip')

        # Clean up staging dir
        shutil.rmtree(staging_dir)

    make_staging_dirs()

    # Package UE4SS Standard
    package_release(is_dev_release=False)
    package_release(is_dev_release=True)

    # CustomGameConfigs
    shutil.make_archive(os.path.join(release_output, 'zCustomGameConfigs'), 'zip', 'assets/CustomGameConfigs')

    # MapGenBP
    shutil.make_archive(os.path.join(release_output, 'zMapGenBP'), 'zip', 'assets/MapGenBP')

    changelog = parse_changelog()
    with open(os.path.join(release_output, 'release_notes.md'), 'w') as file:
        file.write(changelog[0]['notes'])

    print('Done')

commands = {f.__name__: f for f in [
    get_release_notes,
    package,
    release_commit,
]}

def main():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(dest='command', required=True)
    
    package_parser = subparsers.add_parser('package')
    package_parser.add_argument('-e', action='store_true')
    package_parser.add_argument('-d', action='store')
    
    release_commit_parser = subparsers.add_parser('release_commit')
    release_commit_parser.add_argument('username', nargs='?')
    
    args = parser.parse_args()
    commands[args.command](args)

if __name__ == "__main__":
    main()