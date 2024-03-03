#!/usr/bin/env python3

import re
import os
import shutil
import subprocess
import argparse
from datetime import datetime
from template import EnhancedTemplate

# change dir to repo root
os.chdir(os.path.join(os.path.dirname(__file__), '..', '..'))

# outputs to github env if present
def github_output(name, value):
    if 'GITHUB_OUTPUT' in os.environ:
        with open(os.environ['GITHUB_OUTPUT'], 'a') as env:
            env.write(f'{name}={value}')

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

def get_os_files(os):
    if os == 'win64':
        return {
            "dev": ['ue4ss.pdb','ue4ss.dll','dwmapi.dll'],
            "release": ['ue4ss.dll', 'dwmapi.dll']
        }
    elif os == 'linux64':
        return {
            "dev": ['libue4ss.so', 'libue4ss.so.debug'],
            "release": ['libue4ss.so']
        }
    else:
        raise Exception(f'unknown os: {os}')

def package(args):
    is_experimental = args.e

    release_output = 'release'
    shutil.rmtree(release_output, ignore_errors=True)
    os.mkdir(release_output)

    staging_dev = os.path.join(release_output, 'StagingDev')
    staging_release = os.path.join(release_output, 'StagingRelease')

    def make_staging_dirs():
        # builds a release version of /assets by copying the directory and then
        # removing and disabling dev-only settings and files
        exclude_files = [
            'API.txt',
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

        settings_to_modify_in_dev = {
            'GuiConsoleVisible': 1,
            'ConsoleEnabled': 1,
            'EnableHotReloadSystem': 1,
            'IgnoreEngineAndCoreUObject': 0,
            'MaxMemoryUsageDuringAssetLoading': 85,
            'GUIUFunctionCaller': 1,
        }

        change_modstxt = {
            'LineTraceMod': 0,
        }

        # copy whole directory
        shutil.copytree('assets', staging_dev)
        shutil.copytree('assets', staging_release)

        # include repo README
        shutil.copy('README.md', os.path.join(staging_dev, 'README.md'))
        shutil.copy('README.md', os.path.join(staging_release, 'README.md'))

        # remove files
        for file in exclude_files:
            path = os.path.join(staging_release, file)
            try:
                os.remove(path)
            except:
                shutil.rmtree(path)

        # change UE4SS-settings.ini
        config_path_dev = os.path.join(staging_dev, 'UE4SS-settings.ini')
        config_path_release = os.path.join(staging_release, 'UE4SS-settings.ini')
        
        with open(config_path_release, mode='r', encoding='utf-8-sig') as file:
            content_template = EnhancedTemplate(file.read())
        
        # apply settings
        combined_variables = {**settings_to_modify_in_release, "release_type": "release", "os": args.os}
        content_release = content_template.substitute(combined_variables)

        combined_variables = {**settings_to_modify_in_dev, "release_type": "dev", "os": args.os}
        content_dev = content_template.substitute(combined_variables)

        with open(config_path_release, mode='w', encoding='utf-8-sig') as file:
            file.write(content_release)

        with open(config_path_dev, mode='w', encoding='utf-8-sig') as file:
            file.write(content_dev)

        # change Mods/mods.txt
        mods_path = os.path.join(staging_release, 'Mods/mods.txt')

        with open(mods_path, mode='r', encoding='utf-8-sig') as file:
            content = file.read()

        for key, value in change_modstxt.items():
            pattern = rf'(^{key}\s*:).*?$'
            content = re.sub(pattern, rf'\1 {value}', content, flags=re.MULTILINE)

        with open(mods_path, mode='w', encoding='utf-8-sig') as file:
            file.write(content)

    def package_release(is_dev_release: bool):
        try:
            version = subprocess.check_output(['git', 'describe', '--tags']).decode('utf-8').strip()
        except:
            version = '0.0.0'
        if is_dev_release:
            os_files = get_os_files(args.os)['dev']
            main_zip_name = f'zDEV-UE4SS_{version}'
            staging_dir = staging_dev
        else:
            os_files = get_os_files(args.os)['release']
            main_zip_name = f'UE4SS_{version}'
            staging_dir = staging_release
        
        target_paths = []
        scan_start_dir = '.'
        if str(args.d) != 'None':
            scan_start_dir = str(args.d)

        for root, dirs, files in os.walk(scan_start_dir):
            for file in files:
                if file.lower() in os_files:
                    target_paths.append(os.path.join(root, file))

        for target in target_paths:
            shutil.copy(target, staging_dir)

        if is_dev_release:
            if os.path.exists(os.path.join(staging_dir, 'docs')):
                shutil.copytree('docs', os.path.join(staging_dir, 'docs'))

        output = os.path.join(release_output, main_zip_name)
        shutil.make_archive(output, 'zip', staging_dir)
        print(f'created package {output}.zip')

        # clean up
        for bin in ['ue4ss.dll', 'dwmapi.dll', 'ue4ss.pdb']:
            try:
                os.remove(os.path.join(staging_dir, bin))
            except:
                pass

        shutil.rmtree(os.path.join(staging_dir, 'docs'), ignore_errors=True)


    make_staging_dirs();

    # Package UE4SS Standard
    package_release(is_dev_release = False)
    package_release(is_dev_release = True)

    # CustomGameConfigs
    shutil.make_archive(os.path.join(release_output, 'zCustomGameConfigs'), 'zip', 'assets/CustomGameConfigs')

    # MapGenBP
    shutil.make_archive(os.path.join(release_output, 'zMapGenBP'), 'zip', 'assets/MapGenBP')

    changelog = parse_changelog()
    with open(os.path.join(release_output, 'release_notes.md'), 'w') as file:
        file.write(changelog[0]['notes'])

    print('done')

commands = {f.__name__: f for f in [
    get_release_notes,
    package,
    release_commit,
]}

parser = argparse.ArgumentParser()
subparsers = parser.add_subparsers(dest='command', required=True)
package_parser = subparsers.add_parser('package')
package_parser.add_argument('-e', action='store_true')
package_parser.add_argument('-d', action='store')
# -s for operating system
package_parser.add_argument('-s', '--os', action='store', default='win64', choices=['win64', 'linux64'])
release_commit_parser = subparsers.add_parser('release_commit')
release_commit_parser.add_argument('username', nargs='?')
args = parser.parse_args()

commands[args.command](args)
