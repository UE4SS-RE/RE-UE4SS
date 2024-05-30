import json
import sys

def generate_warning_message(version_config):
    """Generates a warning message template that is used to warn uses on each page and provide
    an optional redirect link. The intended use is to warn users that they are viewing a non-release
    version of the docs and optionally link them to the release docs.

    Args:
        version_config (config): versionwarning config which is loaded from book.toml
        Usage of this config is documented in book.toml

    Returns:
        string: Warning template to apply to the top of each page. You can optionally return None
        if you don't want any warnings applied to each page.
    """
    header = '<div id="version-warning" class="warning">'
    if version_config['version'] == "local":
        header += "You are viewing locally generated documentation."
    elif version_config['version'] == "branch":
        header += """You are viewing the <strong>{branch}</strong> version of the UE4SS documentation. 
                The API and features are subject to change at any time. 
                If you are developing mods for UE4SS, you should reference 
                the <a id="redirect-url" href="{url}/{{path}}">latest release docs</a> instead.""".format(
                    branch = version_config['branch'],
                    url = version_config['redirect-url'].rstrip('\\'))
    elif version_config['version'] == "tag":
        header += """You are viewing the <strong>{tag}</strong> version of the UE4SS documentation. 
                If you *are* developing mods for the latest version of UE4SS, you should reference 
                the <a id="redirect-url" href="{url}/{{path}}">latest release docs</a> instead.""" .format(
                    tag = version_config['tag'],
                    url = version_config['redirect-url'].rstrip('\\'))
    elif version_config['version'] == "release":
        return None
    else:
        header += "This documentation was generated with an unparseable version_config: {dump}".format(dump = json.dumps(version_config))
    header += '</div>\r\n\r\n'
    return header

def append_to_chapter(chapter, header):
    """Appends a header to a page and recursively applies it to any sub chapters belonging to the page.

    Args:
        chapter (mdbook_section): Section from reading stdin from mdbook.
        header (string): The header to apply to the beginning of each page.
        The {path} string in the header will try to be replaced with chapter['path'] in order to
        facilitate the redirection to release docs from dev/previous version docs.
    """
    chapter['content'] = str(header).format(path=chapter['path']) + chapter['content']
    for sub_chapter in chapter['sub_items']:
        append_to_chapter(sub_chapter['Chapter'], header)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        if sys.argv[1] == "supports": 
            sys.exit(0)

    # mdbook preprocessor passes two json payloads to us.
    # We use `context` to determine if we're building for a branch/tag/locally.
    # We use `book` to modify the actual contents of the book.
    context, book = json.load(sys.stdin)

    # Load the header template based on our passed in `versionwarning` config.
    # See book.toml for specifics about how to set the config from CLI/scripts.
    header_template = generate_warning_message(context['config']['preprocessor']['versionwarning'])

    # If the version we're building doesn't need any warning header applied to it.
    if header_template is not None:
        for section in book['sections'] :
            if 'Chapter' in section: # Skip over empty sections.
                append_to_chapter(section['Chapter'], header_template)
            


    # and now, we can just modify the content of the first chapter
    # we are done with the book's modification, we can just print it to stdout, 
    print(json.dumps(book, ensure_ascii=False))


