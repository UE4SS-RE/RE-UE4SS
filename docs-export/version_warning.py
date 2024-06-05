import json
import sys


def append_to_chapter(chapter, header):
    """Append a header to a page and recursively apply it to any sub chapters.

    Args:
        chapter (mdbook_section): Section from reading stdin from mdbook.
        header (string): The header to apply to the beginning of each page.
        The {path} string in the header will try to be replaced with chapter['path'] in order to
        facilitate the redirection to release docs from dev/previous version docs.
    """

    formatted_header = str(header).format(path=chapter['path'])
    chapter['content'] =  + chapter['content']
    for sub_chapter in chapter['sub_items']:
        append_to_chapter(sub_chapter['Chapter'], header)


if __name__ == '__main__':
    if len(sys.argv) > 1:
        if sys.argv[1] == 'supports':
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
        for section in book['sections']:
            if 'Chapter' in section:  # Skip over empty sections.
                append_to_chapter(section['Chapter'], header_template)

    sys.stdout.write(json.dumps(book, ensure_ascii=False))
    sys.stdout.flush()
