import re
from pathlib import Path

path = Path("g:/Project/PML/tests/test_evaluator.cpp")
text = path.read_text(encoding="utf-8")

replacements = [
    (r'std::holds_alternative<std::shared_ptr<pml::ValueList>>\(\*result\)', 'result->is_list()'),
    (r'std::get<std::shared_ptr<pml::ValueList>>\(\*result\)', '*result->as_list()'),
    (r'std::get<int64_t>\(list->elements\[([0-9]+)\]\)', r'list->elements[\1].int_val()'),
]

for pat, repl in replacements:
    text = re.sub(pat, repl, text)

path.write_text(text, encoding="utf-8")
print("done")
