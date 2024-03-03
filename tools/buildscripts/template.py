import string
import ast
import operator

class SafeEvaluator:
    """A simple and safe expression evaluator supporting basic operations."""
    ALLOWED_OPERATORS = {
        ast.Add: operator.add,
        ast.Sub: operator.sub,
        ast.Mult: operator.mul,
        ast.Div: operator.truediv,
        ast.Mod: operator.mod,
        ast.Pow: operator.pow,
        ast.Eq: operator.eq,
        ast.NotEq: operator.ne,
        ast.Lt: operator.lt,
        ast.LtE: operator.le,
        ast.Gt: operator.gt,
        ast.GtE: operator.ge,
        ast.And: operator.and_,
        ast.Or: operator.or_,
        ast.Not: operator.not_,
    }
    
    def evaluate(self, expression, variables):
        """Evaluate an expression safely."""
        try:
            tree = ast.parse(expression, mode='eval')
            return self._eval(tree.body, variables)
        except:
            return False
    
    def _eval(self, node, variables):
        if isinstance(node, ast.Expression):
            return self._eval(node.body, variables)
        elif isinstance(node, ast.BoolOp):
            op = self.ALLOWED_OPERATORS[type(node.op)]
            return op(*[self._eval(value, variables) for value in node.values])
        elif isinstance(node, ast.BinOp):
            return self.ALLOWED_OPERATORS[type(node.op)](self._eval(node.left, variables), self._eval(node.right, variables))
        elif isinstance(node, ast.UnaryOp):
            return self.ALLOWED_OPERATORS[type(node.op)](self._eval(node.operand, variables))
        elif isinstance(node, ast.Compare):
            left = self._eval(node.left, variables)
            right = self._eval(node.comparators[0], variables)
            return self.ALLOWED_OPERATORS[type(node.ops[0])](left, right)
        elif isinstance(node, ast.Num):
            return node.n
        elif isinstance(node, ast.Name):
            return variables.get(node.id, False)
        elif isinstance(node, ast.Constant):
            return node.value
        else:
            return False
import re
class EnhancedTemplate(string.Template):
    delimiter = '$'
    
    def substitute(self, mapping):
        template = self.template
        evaluator = SafeEvaluator()
        processed_template = self._process_conditionals(template, mapping, evaluator)
        return string.Template(processed_template).substitute(mapping)

    def _active(self, cond):
        for c, _taken, _kw in cond:
            if c == False:
                return False
        return True

    def _process_conditionals(self, template, mapping, evaluator):
        # Improved regex to include elsif and else
        pattern = re.compile(r'\$\{(if|elsif|else)(.*?)}\r?\n?|\$\{endif\}\r?\n?')
        blocks = []
        cursor = 0
        skip_block = False
        conditions = []
        for match in pattern.finditer(template):
            start, end = match.span()
            keyword, condition = match.groups()
            
            # Add non-conditional text
            if cursor < start:
                if self._active(conditions):
                    blocks.append(template[cursor:start])

            if keyword == 'if':
                cursor = end
                cond = evaluator.evaluate(condition.strip(), mapping)
                # active, taken
                conditions.append((cond, cond, 'if'))
            elif keyword == 'elsif':
                if len(conditions) == 0:
                    raise ValueError("elseif without a matching if")
                c, taken, kw = conditions[-1]
                if kw == 'else':
                    raise ValueError("elseif after else")
                if not taken:
                    cond = evaluator.evaluate(condition.strip(), mapping)
                    conditions[-1] = (cond, cond, 'elsif')
                else:
                    conditions[-1] = (False, True, 'elsif')
            elif keyword == 'else':
                if len(conditions) == 0:
                    raise ValueError("else without a matching if")
                c, taken, kw = conditions[-1]
                if not taken:
                    conditions[-1] = (True, True, 'else')
                else:
                    conditions[-1] = (False, True, 'else')
                cursor = end
                continue
            elif match.group(0).strip() == '${endif}':  
                if len(conditions) == 0:
                    raise ValueError("endif without a matching if")
                conditions.pop()
                cursor = end
                continue

            if self._active(conditions):
                blocks.append(template[end:match.end()])

            cursor = match.end()

        if len(conditions) > 0:
            print(conditions)
            raise ValueError("Unmatched if")
        # Add any remaining template content
        if cursor < len(template):
            blocks.append(template[cursor:])

        return ''.join(blocks)



def test():
    test_cases = [
        # Simple if
        {"template": "Test ${if True}passed${endif}.", "variables": {}, "expected": "Test passed."},
        # if with variable
        {"template": "${if condition}Condition met.${endif}", "variables": {"condition": True}, "expected": "Condition met."},
        # if-else (with elsif used as else)
        {"template": "${if condition}True.${elsif True}False.${endif}", "variables": {"condition": False}, "expected": "False."},
        # Unmatched condition
        {"template": "Always visible ${if condition}Condition met.${endif}", "variables": {"condition": False}, "expected": "Always visible "},
        # Complex condition
        {"template": "${if num > 10}Greater.${endif}", "variables": {"num": 15}, "expected": "Greater."},
        # Nested conditions not applicable, adding more varied conditions instead
        # Multiple conditions with and
        {"template": "${if condition1 and condition2}Both true.${endif}", "variables": {"condition1": True, "condition2": True}, "expected": "Both true."},
        # Multiple conditions with or
        {"template": "${if condition1 or condition2}At least one true.${endif}", "variables": {"condition1": False, "condition2": True}, "expected": "At least one true."},
        # Inline if
        {"template": "Inline ${if condition}passed${endif}, indeed.", "variables": {"condition": True}, "expected": "Inline passed, indeed."},
        # if-elsif-endif
        {"template": "${if condition1}First.${elsif condition2}Second.${endif}", "variables": {"condition1": False, "condition2": True}, "expected": "Second."},
        # if-elsif-elsif-endif
        {"template": "${if condition1}First.${elsif condition2}Second.${elsif condition3}Third.${endif}", "variables": {"condition1": False, "condition2": False, "condition3": True}, "expected": "Third."},
        # if-false
        {"template": "${if condition}True.${endif}", "variables": {"condition": False}, "expected": ""},
        # Expression with variables
        {"template": "${if var1 + var2 == 10}Sum is 10.${endif}", "variables": {"var1": 3, "var2": 7}, "expected": "Sum is 10."},
        # Inequality
        {"template": "${if var != 5}Not five.${endif}", "variables": {"var": 3}, "expected": "Not five."},
        # Greater than or equal
        {"template": "${if var >= 5}Greater or equal.${endif}", "variables": {"var": 5}, "expected": "Greater or equal."},
        # Less than
        {"template": "${if var < 5}Less.${endif}", "variables": {"var": 3}, "expected": "Less."},
        # Not condition
        {"template": "${if not condition}Not True.${endif}", "variables": {"condition": False}, "expected": "Not True."},
        # And + Or
        {"template": "${if condition1 and (condition2 or condition3)}Complex condition met.${endif}", "variables": {"condition1": True, "condition2": False, "condition3": True}, "expected": "Complex condition met."},
        # Inline with text around
        {"template": "Start ${if condition}middle${endif} end.", "variables": {"condition": True}, "expected": "Start middle end."},
        # Using else (via elsif as a workaround)
        {"template": "${if condition}First.${elsif True}Else.${endif}", "variables": {"condition": False}, "expected": "Else."},
        # Multiple elseif
        {"template": "${if condition1}One.${elsif condition2}Two.${elsif condition3}Three.${endif}", "variables": {"condition1": False, "condition2": False, "condition3": True}, "expected": "Three."},
        # Fallback else
        {"template": "${if condition1}One.${elsif condition2}Two.${elsif True}Default.${endif}", "variables": {"condition1": False, "condition2": False}, "expected": "Default."},
        {
        "template": """Before conditional block.
${if condition1}
Line 1: Condition 1 is true.
${elsif condition2}
Line 2: Condition 2 is true.
    Further line still within Condition 2 block.
${elsif condition3}
Line 3: Condition 3 is true.
${else}
None of the conditions were met.
${endif}
After conditional block.""",
        "variables": {"condition1": False, "condition2": True, "condition3": False},
        "expected": """Before conditional block.
Line 2: Condition 2 is true.
    Further line still within Condition 2 block.
After conditional block."""
        },
        {
            "template": """
${if condition1}
  ${if conditionA}A is true.${else}A is false.${endif}
${elsif condition2}
  ${if conditionB}
    ${if conditionX}B and X are true.${elsif True}B is true, X is default.${endif}
  ${else}
    B is false.
  ${endif}
${else}
  ${if conditionC}
    C is true.
  ${else}
    ${if conditionY}Y is true, but C is false.${else}Default case.${endif}
  ${endif}
${endif}
""",
            "variables": {
                "condition1": False,
                "condition2": True,
                "conditionA": False,
                "conditionB": True,
                "conditionC": False,
                "conditionX": False,
                "conditionY": False
        },
        "expected": """
  B is true, X is default."""
        },
        {
            "template": """
${if condition1}
  Level 1: Condition 1 is true.
  ${if conditionA}
    Level 2: Condition A is true.
    ${if conditionX}Level 3: X.${elsif conditionY}Level 3: Y.${else}Level 3: Neither X nor Y.${endif}
  ${elsif conditionB}
    Level 2: Condition B is true.
    ${if conditionZ}Level 3: Z is true.${else}Level 3: Z is false.${endif}
  ${else}
    Level 2: Neither A nor B is true.
  ${endif}
${elsif condition2}
  Level 1: Condition 2 is true.
  ${if conditionC}
    Level 2: Condition C is true.
  ${else}
    Level 2: Condition C is false.
    ${if conditionW}Level 3: W.${else}Level 3: Not W.${endif}
  ${endif}
${else}
  Level 1: Neither condition 1 nor 2 is true.
  ${if conditionD}
    Level 2: D is true. Default path.
  ${else}
    Level 2: D is false. Ultimate fallback.
  ${endif}
${endif}
""",
            "variables": {
                "condition1": False,
                "condition2": True,
                "conditionA": False,
                "conditionB": False,
                "conditionC": False,
                "conditionX": False,
                "conditionY": False,
                "conditionZ": False,
                "conditionW": True,
                "conditionD": False
            },
            "expected": """
  Level 1: Condition 2 is true.
    Level 2: Condition C is false.
    Level 3: W."""
        }
    ]
    def normalize_space(s):
        # Replace multiple spaces with a single space
        s = re.sub(r'[ ]+', ' ', s)
        # Replace multiple newlines with a single newline
        s = re.sub(r'(\n\s*)+\n', '\n', s)
        # Trim leading and trailing whitespace
        s = s.strip()
        return s
    def compare_strings(s1, s2):
        normalized_s1 = normalize_space(s1)
        normalized_s2 = normalize_space(s2)
        
        return normalized_s1 == normalized_s2
    for i, test in enumerate(test_cases, 1):
        template = EnhancedTemplate(test["template"])
        output = template.substitute(test["variables"])
        assert compare_strings(output.strip(), test["expected"].strip()), f"Test {i} failed. \n\tTemplate: '{test['template']}'\n\tInput: {test['variables']}\n\tExpected: '{test['expected']}',\n\tGot: '{output}'"
        print(f"Test {i} passed.")

if __name__ == "__main__":
    test()