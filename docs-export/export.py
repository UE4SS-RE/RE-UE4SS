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

    # take README.md from the exported ref so each version shows its own instructions
    # rewrite absolute URLs to relative mdBook URLs
    readme = subprocess.check_output(['git', 'show', f'{ref}:README.md']).decode('utf-8')
    readme = re.sub(r'\(https://docs.ue4ss.com/dev/([^)#]+)\.html(#[^)]*)?\)', r'(\1.md\2)', readme)
    lines = readme.splitlines(keepends=True)

    # the release docs fall back to the main branch when no tag has docs
    # only a tag has a release page to link to and a ref worth pinning
    is_tag = ref in tags
    tag_link = f'[{ref}](https://github.com/UE4SS-RE/RE-UE4SS/releases/tag/{ref})'

    # insert banner depending on version
    banner = None
    if name == 'dev':
        banner = '> [!WARNING]\n> This is the dev version of the UE4SS docs, which reflects the experimental version (which is the main branch of the repository). The API and features are subject to change at any time. If you are developing mods for UE4SS, you should reference the [latest release](../release) instead.'
    elif is_tag and ref == tags[-1]:
        # also covers the latest tag's own versioned page
        banner = f'> [!NOTE]\n> This is the stable release version of the UE4SS docs, which reflects state at release tag {tag_link}, which is recommended for developing mods on. If you prefer to build against the experimental version (which is the main branch of the repository), you should reference the [dev documentation](../dev) instead.'
    elif is_tag:
        banner = f'> [!WARNING]\n> This is an old version of the UE4SS docs, which reflects the release tag of {tag_link}. The API and features may have changed in a newer version, so you may want to consider updating. The stable version of the UE4SS docs can be found [here](../release).'
    if banner:
        lines.insert(2, f'\n{banner}\n\n')

    # for tagged versions, make the build instructions clone the matching tag instead of the main branch
    if is_tag:
        for i, line in enumerate(lines):
            if line.strip().startswith('1. Clone the repo.'):
                lines[i] = f'1. Clone the repo from the tag `{ref}` using one of the following methods:\n\n' \
                           f'   1. Clone the main branch and check out the tag:\n' \
                           f'      ```\n' \
                           f'      git clone https://github.com/UE4SS-RE/RE-UE4SS.git\n' \
                           f'      cd RE-UE4SS\n' \
                           f'      git checkout {ref}\n' \
                           f'      ```\n' \
                           f'   2. Clone the tag directly:\n' \
                           f'      ```\n' \
                           f'      git clone --branch {ref} https://github.com/UE4SS-RE/RE-UE4SS.git\n' \
                           f'      ```\n\n'
                break
        else:
            # shows up as an annotation on the docs deploy run
            print(f'::warning::No "1. Clone the repo." step found in README.md at {ref}, clone instructions were left unchanged')

    with open(os.path.join(src_dir, 'README.md'), 'w', encoding='utf-8') as file:
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
