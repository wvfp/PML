import re
from pathlib import Path

ROOT = Path("g:/Project/PML/src/pml")
FILES = [
    "sprites/components/scene_elements.cpp",
    "sprites/components/outfit.cpp",
    "sprites/components/mouth.cpp",
    "sprites/components/items.cpp",
    "sprites/components/head.cpp",
    "sprites/components/hair.cpp",
    "sprites/components/eyes.cpp",
    "sprites/components/character.cpp",
    "sprites/components/body.cpp",
    "sprites/registry.cpp",
]

def transform(text: str) -> str:
    # std::get<std::string>(p["X"])
    text = re.sub(
        r'std::get<std::string>\(p\["([^"]+)"\]\)',
        r'*p["\1"].as_string()',
        text,
    )
    # std::get<double>(p["X"])
    text = re.sub(
        r'std::get<double>\(p\["([^"]+)"\]\)',
        r'p["\1"].double_val()',
        text,
    )
    # std::get<bool>(p["X"])
    text = re.sub(
        r'std::get<bool>\(p\["([^"]+)"\]\)',
        r'p["\1"].bool_val()',
        text,
    )
    # std::get_if<T>(&var)  -> var.as_*()
    text = re.sub(
        r'std::get_if<std::string>\(&([a-zA-Z_][a-zA-Z0-9_]*)\)',
        r'\1.as_string()',
        text,
    )
    text = re.sub(
        r'std::get_if<Symbol>\(&([a-zA-Z_][a-zA-Z0-9_]*)\)',
        r'\1.as_symbol()',
        text,
    )
    text = re.sub(
        r'std::get_if<Keyword>\(&([a-zA-Z_][a-zA-Z0-9_]*)\)',
        r'\1.as_keyword()',
        text,
    )
    text = re.sub(
        r'std::get_if<std::shared_ptr<GraphicObject>>\(&([a-zA-Z_][a-zA-Z0-9_]*)\)',
        r'\1.as_graphic_object()',
        text,
    )
    # std::holds_alternative<std::nullptr_t>(expr)
    text = re.sub(
        r'std::holds_alternative<std::nullptr_t>\(([^)]+)\)',
        r'\1.is_nil()',
        text,
    )
    # std::get_if<double>(&obj->metadata.at("key"))
    def metadata_repl(m):
        obj = m.group(1)
        key = m.group(2)
        return f'{obj}->metadata.at("{key}").is_double()'
    text = re.sub(
        r'std::get_if<double>\(&([a-zA-Z_][a-zA-Z0-9_]*)->metadata\.at\("([^"]+)"\)\)',
        metadata_repl,
        text,
    )
    return text

for rel in FILES:
    path = ROOT / rel
    original = path.read_text(encoding="utf-8")
    changed = transform(original)
    if changed != original:
        path.write_text(changed, encoding="utf-8")
        print(f"updated {rel}")
    else:
        print(f"no change {rel}")
