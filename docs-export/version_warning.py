import json
import sys

def generate_warning_message(version_config):
    header = '<div id="version-warning" class="warning">'
    if version_config['version'] == "local":
        header += "You are viewing locally generated documentation."
    elif version_config['version'] == "branch":
        header += """You are viewing the <strong>%s</strong> version of the UE4SS documentation. 
                The API and features are subject to change at any time. 
                If you are developing mods for UE4SS, you should reference 
                the <a id="redirect-url" href="%s">latest release docs</a> instead.""" % (version_config['branch'], version_config['redirect-url'].rstrip('\\'))
    elif version_config['version'] == "tag":
        header += """You are viewing the <strong>%s</strong> version of the UE4SS documentation. 
                If you are developing mods for the latest version of UE4SS, you should reference 
                the <a id="redirect-url" href="%s">latest release docs</a> instead.""" % (version_config['tag'], version_config['redirect-url'].rstrip('\\'))
    elif version_config['version'] == "release":
        return None
    else:
        header += "This documentation was generated with an unparseable version_config: %s" % json.dumps(version_config)
    header += '</div>\r\n\r\n'
    return header

    
def append_to_chapter(chapter, append_str):
    chapter['content'] = append_str + chapter['content']
    for sub_chapter in chapter['sub_items']:
        append_to_chapter(sub_chapter['Chapter'], append_str)

if __name__ == '__main__':
    if len(sys.argv) > 1:
        if sys.argv[1] == "supports": 
            sys.exit(0)

    # load both the context and the book representations from stdin
    context, book = json.load(sys.stdin)
    


    header = generate_warning_message(context['config']['preprocessor']['versionwarning'])
    if header is not None:
        for section in book['sections'] :
            append_to_chapter(section['Chapter'], header)
            


    # and now, we can just modify the content of the first chapter
    # we are done with the book's modification, we can just print it to stdout, 
    print(json.dumps(book, ensure_ascii=False))


