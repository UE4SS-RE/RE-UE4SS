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

    readme_spec = f'{ref}:README.md'
    readme_out_path = os.path.join(src_dir, 'README.md')

    # retrieve the README.md file from the specific ref
    readme_content = subprocess.check_output(['git', 'show', readme_spec], text=True)

    # rewrite absolute URLs to relative mdbook URLs
    transformed_content = re.sub(r'\(https://docs.ue4ss.com/dev/([^)#]+)\.html(#[^)]*)?\)', r'(\1.md\2)', readme_content)

    with open(readme_out_path, 'w') as file:
        file.write(transformed_content)

    with open(readme_out_path, 'r') as file:
        lines = file.readlines()
    
    # insert banner depending on version
    if name == "release":
        lines.insert(2, f'\n> [!CAUTION]\n> This is the stable release version of the UE4SS docs, which reflects state at release tag [{ref}](https://github.com/UE4SS-RE/RE-UE4SS/releases/tag/{ref}), which is recommended for developing mods on. If you prefer to build against the experimental version (which is the main branch of the repository), you should reference the [dev documentation](../dev) instead.\n\n')
    elif name == "dev":
        lines.insert(2, f'\n> [!CAUTION]\n> This is the dev version of the UE4SS docs, which reflects the experimental version (which is the main branch of the repository). The API and features are subject to change at any time. If you are developing mods for UE4SS, you should reference the [latest release](../release) instead.\n\n')
    else:
        lines.insert(2, f'\n> [!CAUTION]\n> This is an old version of the UE4SS docs, which reflects the release tag of [{ref}](https://github.com/UE4SS-RE/RE-UE4SS/releases/tag/{ref}). The API and features may have changed in a newer version, so you may want to consider updating. The stable version of the UE4SS docs can be found [here](../release).\n\n')

    # for readmes on non-dev versions, edit the readme to make sure people clone the correct tag of the repo to build against
    if name != "dev":
        for i, line in enumerate(lines):
            if line.strip().startswith("1. Clone the repo."):
                lines[i] = f"1. Clone the repo from the tag `{ref}` using one of the following methods:\n\n" \
                        f"   1. Clone the main branch and check out the tag:\n" \
                        f"     ```\n" \
                        f"     git clone https://github.com/UE4SS-RE/RE-UE4SS.git\n" \
                        f"     cd RE-UE4SS\n" \
                        f"     git checkout {ref}\n" \
                        f"     ```\n" \
                        f"   2. Clone the tag directly:\n" \
                        f"     ```\n" \
                        f"     git clone --branch {ref} https://github.com/UE4SS-RE/RE-UE4SS.git\n" \
                        f"     ```\n\n"
                break
    
    with open(readme_out_path, 'w') as file:
        file.writelines(lines)

    # copy template files
    shutil.copy(os.path.join('docs-export', 'book.toml'), version_out_dir)
    shutil.copytree(os.path.join('docs-export', 'css'), os.path.join(version_out_dir, 'css'))

    # github alerts pre-processor template files, if they are not found for whatever reason, just ignore
    try:
        shutil.copy(os.path.join('docs-export', 'mermaid.min.js'), version_out_dir)
        shutil.copy(os.path.join('docs-export', 'mermaid-init.js'), version_out_dir)
    except FileNotFoundError:
        pass

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
