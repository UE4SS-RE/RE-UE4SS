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

def apply_book_settings(content, language, title):
    if language:
        content = re.sub(r'^language = ".*"$', f'language = "{language}"', content, flags=re.MULTILINE)
    if title:
        content = re.sub(r'^title = ".*"$', f'title = "{title}"', content, flags=re.MULTILINE)
    return content


def merge_untranslated(src_dir, fallback_dir):
    """Copy files that are missing from a translation over from the English docs.

    A translation only ships the pages that have been translated so far, but its
    SUMMARY.md may still reference every page. Filling the gaps with the source
    language keeps the book buildable while the translation is in progress.
    """
    for root, _, files in os.walk(fallback_dir):
        rel_dir = os.path.relpath(root, fallback_dir)
        dst_dir = src_dir if rel_dir == '.' else os.path.join(src_dir, rel_dir)
        os.makedirs(dst_dir, exist_ok=True)
        for file_name in files:
            dst_file = os.path.join(dst_dir, file_name)
            if not os.path.exists(dst_file):
                shutil.copy2(os.path.join(root, file_name), dst_file)


def export_version(ref, name, docs_path='docs', language=None, title=None, fallback_path=None):
    docs_spec = f'{ref}:{docs_path}'

    # old tags do not have a 'docs' directory so check and fail if it does not exist
    try:
        subprocess.check_output(['git', 'show', docs_spec], stderr=subprocess.DEVNULL)
        from_git = True
    except subprocess.CalledProcessError:
        # A source directory that only exists in the working tree (for example a
        # translation that has not been committed yet) can still be exported so
        # that it can be previewed locally.
        if not os.path.isdir(docs_path):
            return False
        from_git = False

    version_out_dir = os.path.join(versions_out_dir, name)
    os.mkdir(version_out_dir)

    # export the docs directory via 'git archive' to <version>/src/
    src_dir = os.path.join(version_out_dir, 'src')
    os.mkdir(src_dir)
    if from_git:
        ps = subprocess.Popen(('git', 'archive', docs_spec), stdout=subprocess.PIPE)
        subprocess.check_output(('tar', 'xf', '-', '-C', src_dir), stdin=ps.stdout)
        ps.wait()
    else:
        shutil.copytree(docs_path, src_dir, dirs_exist_ok=True)

    # fill in the pages that have not been translated yet
    if fallback_path is not None:
        merge_untranslated(src_dir, fallback_path)

    # Deduplicate SUMMARY.md for old versions with duplicate paths
    # mdBook v0.5+ enforces unique paths, but old release tags may have duplicates
    # This preprocessing step removes duplicate entries before mdBook sees them
    summary_path = os.path.join(src_dir, 'SUMMARY.md')
    deduplicate_summary(summary_path)

    # copy README.md to <version>/src/
    # rewrite absolute URLs to relative mdBook URLs
    # translated docs may ship their own README.md which takes precedence
    readme_src = 'README.md'
    translated_readme = os.path.join(docs_path, 'README.md')
    if docs_path != 'docs' and os.path.exists(translated_readme):
        readme_src = translated_readme
    copy_transform(readme_src, os.path.join(src_dir, 'README.md'), lambda content: re.sub(r'\(https://docs.ue4ss.com/dev/([^)#]+)\.html(#[^)]*)?\)', r'(\1.md\2)', content))

    # insert banner
    if name == 'dev':
        readme_path = os.path.join(src_dir, 'README.md')
        with open(readme_path, 'r') as file:
            lines = file.readlines()
        lines.insert(2, '\n> [!WARNING]\n> WARNING: This is the dev version of the UE4SS docs. The API and features are subject to change at any time. If you are developing mods for UE4SS, you should reference the [latest release](../release) instead.\n\n')
        with open(readme_path, 'w') as file:
            file.writelines(lines)

    # copy template files
    copy_transform(os.path.join('docs-export', 'book.toml'), os.path.join(version_out_dir, 'book.toml'), lambda content: apply_book_settings(content, language, title))
    shutil.copytree(os.path.join('docs-export', 'css'), os.path.join(version_out_dir, 'css'))
    shutil.copytree(os.path.join('docs-export', 'js'), os.path.join(version_out_dir, 'js'))

    # github alerts pre-processor template files, if they are not found for whatever reason, just ignore
    try:
        shutil.copy(os.path.join('docs-export', 'mermaid.min.js'), version_out_dir)
        shutil.copy(os.path.join('docs-export', 'mermaid-init.js'), version_out_dir)
    except FileNotFoundError:
        pass

    return True

export_version('main', 'dev')

# Chinese translation, exported as a sibling of the release book so that the
# language switch button can link between the two books.
export_version('main', 'zh-CN', docs_path='docs-zh-CN', language='zh-CN', title='UE4SS 文档', fallback_path='docs')

found_release = False
if tags:
    found_release = export_version(tags[-1], 'release')

if not found_release:
    # if no existing release than just use tip of main branch to have something
    export_version('main', 'release')

for tag in tags:
    export_version(tag, tag)
