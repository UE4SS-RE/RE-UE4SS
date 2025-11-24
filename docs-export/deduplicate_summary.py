#!/usr/bin/env python3

import os
import sys
import re

def deduplicate_summary(summary_path):
    """
    Remove duplicate file references from SUMMARY.md while preserving structure.
    Keeps the first occurrence of each unique file path.
    """
    if not os.path.exists(summary_path):
        return False

    with open(summary_path, 'r', encoding='utf-8') as f:
        lines = f.readlines()

    seen_paths = set()
    new_lines = []
    modified = False

    # Regex to match markdown links: [text](path)
    link_pattern = re.compile(r'\[([^\]]+)\]\(([^)]+)\)')

    for line in lines:
        # Find all markdown links in the line
        matches = link_pattern.findall(line)

        if matches:
            # Get the file path (remove anchors)
            for text, path in matches:
                # Remove anchor if present
                clean_path = path.split('#')[0]

                if clean_path in seen_paths:
                    # This is a duplicate, comment it out
                    if not line.strip().startswith('<!--'):
                        line = ''
                        modified = True
                else:
                    seen_paths.add(clean_path)

        new_lines.append(line)

    if modified:
        with open(summary_path, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)
        return True

    return False

if __name__ == '__main__':
    if len(sys.argv) > 1:
        summary_path = sys.argv[1]
    else:
        summary_path = 'SUMMARY.md'

    changed = deduplicate_summary(summary_path)
    if changed:
        print(f"Deduplicated {summary_path}")
    else:
        print(f"No duplicates found in {summary_path}")
