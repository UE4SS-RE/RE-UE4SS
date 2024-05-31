#!/usr/bin/env python3

import subprocess
import os
import shutil
import re

# change dir to repo root
os.chdir(os.path.dirname(os.path.dirname(__file__)))

template_repo = 'docs-repo-template'
out_dir = 'docs-repo'
versions_out_dir = os.path.join(out_dir, 'versions')

# get all version tags (prefixed with 'v')
tags = subprocess.check_output(['git', 'tag', '-l', '--sort=v:refname', 'v*']).decode("utf-8").splitlines()

# clear and recreat out directory
shutil.rmtree(out_dir, ignore_errors=True)
shutil.copytree(template_repo, out_dir)
os.mkdir(versions_out_dir)


def copy_transform(src, dst, transformer):
    with open(src, 'r') as file:
        content = file.read()
    content = transformer(content)
    with open(dst, 'w') as file:
        file.write(content)

def export_version(ref, name):
    docs_spec = f'{ref}:docs'

    # old tags do not have a 'docs' directory so check and fail if it does not exist
    try:
        subprocess.check_output(['git', 'show', docs_spec], stderr=subprocess.DEVNULL)
    except subprocess.CalledProcessError:
        return False

    version_out_dir = os.path.join(versions_out_dir, name)
    os.mkdir(version_out_dir)

    # export docs/ via 'git archive' to <version>/src/
    src_dir = os.path.join(version_out_dir, 'src')
    os.mkdir(src_dir)
    ps = subprocess.Popen(('git', 'archive', docs_spec), stdout=subprocess.PIPE)
    subprocess.check_output(('tar', 'xvf', '-', '-C', src_dir), stdin=ps.stdout)
    ps.wait()

    # copy README.md to <version>/src/
    # rewrite absolute URLs to relative mdBook URLs
    copy_transform('README.md', os.path.join(src_dir, 'README.md'), lambda content: re.sub(r'\(https://docs.ue4ss.com/dev/([^)#]+)\.html(#[^)]*)?\)', r'(\1.md\2)', content))

    # insert banner
    if name != 'release':
        readme_path = os.path.join(src_dir, 'README.md')
        with open(readme_path, 'r') as file:
            lines = file.readlines()
        lines.insert(2, '\n> WARNING: This is the dev version of the UE4SS docs. The API and features are subject to change at any time. If you are developing mods for UE4SS, you should reference the [latest release](../release) instead.\n\n')
        with open(readme_path, 'w') as file:
            file.writelines(lines)

    # copy template files
    shutil.copy(os.path.join('docs-export', 'book.toml'), version_out_dir)
    shutil.copytree(os.path.join('docs-export', 'css'), os.path.join(version_out_dir, 'css'))

    return True

export_version('main', 'dev')

found_release = False
if tags:
    found_release = export_version(tags[-1], 'release')

if not found_release:
    # if no existing release than just use tip of main branch to have something
    export_version('main', 'release')

for tag in tags:
    export_version(tag, tag)
