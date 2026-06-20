import re
from pathlib import Path

path = Path("g:/Project/PML/tests/test_modules.cpp")
text = path.read_text(encoding="utf-8")

replacements = [
    (r'std::get<int64_t>\(\*r\)', 'r->int_val()'),
    (r'std::get<bool>\(\*r\)', 'r->bool_val()'),
    (r'std::holds_alternative<bool>\(\*r\)', 'r->is_bool()'),
    (r'std::get<std::shared_ptr<pml::ValueList>>\(\*r\)', '*r->as_list()'),
    (r'std::get<int64_t>\(vl->elements\[([0-9]+)\]\)', r'vl->elements[\1].int_val()'),
    (r'std::get<pml::Symbol>\(vl->elements\[([0-9]+)\]\)', r'*vl->elements[\1].as_symbol()'),
    (r'std::get<pml::Symbol>\(\*r\)', '*r->as_symbol()'),
    (r'std::get<int64_t>\(\*foo_result\)', 'foo_result->int_val()'),
]

for pat, repl in replacements:
    text = re.sub(pat, repl, text)

path.write_text(text, encoding="utf-8")
print("done")
