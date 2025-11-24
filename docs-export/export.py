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

def deduplicate_summary(summary_path):
    """Remove duplicate file references from SUMMARY.md"""
    if not os.path.exists(summary_path):
        return

    with open(summary_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    seen_paths = set()
    new_lines = []
    link_pattern = re.compile(r'\[([^\]]+)\]\(([^)]+)\)')

    for line in lines:
        matches = link_pattern.findall(line)
        is_duplicate = False

        if matches:
            for text, path in matches:
                clean_path = path.split('#')[0]
                if clean_path in seen_paths:
                    # Skip this line entirely instead of commenting it out
                    is_duplicate = True
                    break
                else:
                    seen_paths.add(clean_path)

        # Only add the line if it's not a duplicate
        if not is_duplicate:
            new_lines.append(line)

    with open(summary_path, 'w', encoding='utf-8') as f:
        f.writelines(new_lines)

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

    # Deduplicate SUMMARY.md for old versions with duplicate paths
    # mdBook v0.5+ enforces unique paths, but old release tags may have duplicates
    # This preprocessing step removes duplicate entries before mdBook sees them
    summary_path = os.path.join(src_dir, 'SUMMARY.md')
    deduplicate_summary(summary_path)

    # copy README.md to <version>/src/
    # rewrite absolute URLs to relative mdBook URLs
    copy_transform('README.md', os.path.join(src_dir, 'README.md'), lambda content: re.sub(r'\(https://docs.ue4ss.com/dev/([^)#]+)\.html(#[^)]*)?\)', r'(\1.md\2)', content))

    # insert banner
    if name != 'release':
        readme_path = os.path.join(src_dir, 'README.md')
        with open(readme_path, 'r') as file:
            lines = file.readlines()
        lines.insert(2, '\n> [!WARN]\n> WARNING: This is the dev version of the UE4SS docs. The API and features are subject to change at any time. If you are developing mods for UE4SS, you should reference the [latest release](../release) instead.\n\n')
        with open(readme_path, 'w') as file:
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
