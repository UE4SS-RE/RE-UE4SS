import os
import re

class HeaderFileParser:
    def __init__(self, file_path):
        self.file_path = file_path
        self.parsed_lines = []
        self.ignore_private = False
        self.inside_class = False

    def parse(self):
        with open(self.file_path, 'r') as file:
            lines = file.readlines()

        for line in lines:
            stripped_line = line.strip()

            if self._is_preprocessor_directive(stripped_line):
                continue  # Skip preprocessor directives

            if self._is_curly_brace(stripped_line):
                continue  # Skip curly braces

            if self._is_implemented_code(stripped_line):
                continue  # Skip implemented code

            if self._is_private_specifier(stripped_line):
                self.ignore_private = True
                continue  # Start ignoring lines

            if self._is_public_or_protected_specifier(stripped_line):
                self.ignore_private = False

            if self.ignore_private:
                continue  # Skip lines under private specifier

            if self._is_namespace_declaration(stripped_line):
                self.parsed_lines.append(self._convert_to_header(stripped_line))

            if self._is_struct_or_enum_declaration(stripped_line):
                self.parsed_lines.append(self._convert_to_header(stripped_line))
            elif self._is_class_declaration(stripped_line):
                self.inside_class = True
                self.parsed_lines.append(self._convert_to_header(stripped_line))
            elif self._is_access_specifier(stripped_line) and self.inside_class:
                self.parsed_lines.append(self._convert_to_subheader(stripped_line))
            elif self._is_field_or_method_declaration(stripped_line):
                self.parsed_lines.append(self._convert_to_code_block(stripped_line))
            else:
                continue

    def _is_preprocessor_directive(self, line):
        return line.startswith('#')

    def _is_curly_brace(self, line):
        return line in ['{', '}']

    def _is_implemented_code(self, line):
        return re.search(r'\{.*\}', line) is not None

    def _is_namespace_declaration(self, line):
        return re.match(r'namespace\s+\w+\s*{?', line)

    def _is_struct_or_enum_declaration(self, line):
        return re.match(r'\b(struct|enum)\b', line)

    def _is_class_declaration(self, line):
        return re.match(r'\bclass\b', line)

    def _convert_to_header(self, line):
        if 'namespace' in line:
            return f"## namespace {line.split()[1]}\n\n"
        return f"### {line}\n\n"

    def _is_access_specifier(self, line):
        return line in ["public:", "private:", "protected:"]

    def _is_private_specifier(self, line):
        return line == "private:"

    def _is_public_or_protected_specifier(self, line):
        return line in ["public:", "protected:"]

    def _is_field_or_method_declaration(self, line):
        return re.match(r'[\w\s\*&]+[\w\s\*&]*\([^)]*\);', line) or re.match(r'[\w\s\*&]+[\w\s\*&]*;', line)

    def _convert_to_subheader(self, line):
        return f"#### {line}\n\n"

    def _convert_to_code_block(self, line):
        return f"```cpp\n{line}\n```\n\n"

class MarkdownGenerator:
    def __init__(self, root_dir, output_dir):
        self.root_dir = root_dir
        self.output_dir = output_dir

    def generate(self, file_path, parsed_lines):
        relative_path = self._get_relative_path(file_path)
        file_name = relative_path.replace('.hpp', '.md').replace(os.sep, '.')
        output_path = os.path.join(self.output_dir, file_name)

        with open(output_path, 'w') as md_file:
            md_file.write(f"# {file_name}\n\n")
            for line in parsed_lines:
                md_file.write(line)

    def _get_relative_path(self, path):
        return os.path.relpath(path, self.root_dir)

class DocumentationGenerator:
    def __init__(self, root_dir, output_dir):
        self.root_dir = root_dir
        self.output_dir = output_dir

    def run(self):
        if not os.path.exists(self.output_dir):
            os.makedirs(self.output_dir)

        for subdir, _, files in os.walk(self.root_dir):
            for file in files:
                if file.endswith('.hpp'):
                    file_path = os.path.join(subdir, file)
                    self._process_file(file_path)

    def _process_file(self, file_path):
        parser = HeaderFileParser(file_path)
        parser.parse()
        generator = MarkdownGenerator(self.root_dir, self.output_dir)
        generator.generate(file_path, parser.parsed_lines)

if __name__ == "__main__":
    root = r"F:\\Github Projects\\Other\\RE-UE4SS\\UE4SS\\include\\"
    output_dir = r"F:\\Github Projects\\Other\\RE-UE4SS\docs\\cpp-api\\gen\\"

    doc_generator = DocumentationGenerator(root, output_dir)
    doc_generator.run()
