import json
import sys
import re
import os

if __name__ == '__main__':
    if len(sys.argv) > 1: # we check if we received any argument
        if sys.argv[1] == "supports":
            # then we are good to return an exit status code of 0, since the other argument will just be the renderer's name
            sys.exit(0)

    # load both the context and the book representations from stdin
    context, book = json.load(sys.stdin)
    repo_readme = "../README.md"
    if os.path.isfile(repo_readme):
        with open(repo_readme, 'r') as file:
            content = file.read()
            content = re.sub(r'\(https://docs.ue4ss.com/dev/([^)#]+)\.html(#[^)]*)?\)', r'(\1.md\2)', content)
            book['sections'][0]['Chapter']['content'] = content
    else:
        sys.stderr.write("Unable to find repo readme [%s]. Skipping overriding README.md\r\n" % repo_readme)
        sys.stderr.flush()

    print(json.dumps(book, ensure_ascii=False))
