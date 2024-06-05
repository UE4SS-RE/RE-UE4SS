"""
This is an example script.

test123
"""

import json
import os
import re
import sys

if __name__ == '__main__':
    if len(sys.argv) > 1:  # we check if we received any argument
        if sys.argv[1] == 'supports':
            sys.exit(0)

    # load both the context and the book representations from stdin
    context, book = json.load(sys.stdin)
    repo_readme = '../README.md'

    if os.path.isfile(repo_readme):
        with open(repo_readme, 'r') as readme_stream:
            file_content = readme_stream.read()
            file_content = re.sub(
                r'\(https://docs.ue4ss.com/dev/([^)#]+)\.html(#[^)]*)?\)',
                r'(\1.md\2)',
                file_content,
                )

            book['sections'][0]['Chapter']['content'] = file_content
    else:
        sys.stderr.write((
            'Unable to find repo readme {README}.' +
            'Skipping overriding README.md\r\n'
            ).format(README=repo_readme),
            )
        sys.stderr.flush()

    sys.stdout.write(json.dumps(book, ensure_ascii=False))
    sys.stdout.flush()
