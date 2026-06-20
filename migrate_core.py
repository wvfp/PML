import re
from pathlib import Path

ROOT = Path("g:/Project/PML/src/pml")

FILES = [
    "evaluator/canvas_builtins.cpp",
    "evaluator/transform_builtins.cpp",
    "evaluator/shader_builtins.cpp",
    "evaluator/backend_builtins.cpp",
    "skeleton/ik_ccd.cpp",
    "skeleton/skin_binding.cpp",
    "graphics/render.cpp",
    "graphics/canvas.cpp",
    "backend/skia/skia_backend.cpp",
]

def transform(text: str) -> str:
    # Shared pointer accessors
    text = re.sub(
        r'std::get_if<std::shared_ptr<GraphicObject>>\(&([a-zA-Z0-9_\[\]\->]+)\)',
        r'\1.as_graphic_object()',
        text,
    )
    text = re.sub(
        r'std::get_if<std::shared_ptr<Canvas>>\(&([a-zA-Z0-9_\[\]\->]+)\)',
        r'\1.as_canvas()',
        text,
    )
    text = re.sub(
        r'std::get_if<std::shared_ptr<AffineTransform>>\(&([a-zA-Z0-9_\[\]\->]+)\)',
        r'\1.as_transform()',
        text,
    )
    text = re.sub(
        r'std::get_if<std::shared_ptr<ValueList>>\(&([a-zA-Z0-9_\[\]\->]+)\)',
        r'\1.as_list()',
        text,
    )
    text = re.sub(
        r'std::get_if<std::shared_ptr<SkeletonInstance>>\(&([a-zA-Z0-9_\[\]\->]+)\)',
        r'\1.as_skeleton_instance()',
        text,
    )

    # Symbol / Keyword / String accessors
    text = re.sub(
        r'std::get_if<Symbol>\(&([a-zA-Z0-9_\[\]\->]+)\)',
        r'\1.as_symbol()',
        text,
    )
    text = re.sub(
        r'std::get_if<Keyword>\(&([a-zA-Z0-9_\[\]\->]+)\)',
        r'\1.as_keyword()',
        text,
    )
    text = re.sub(
        r'std::get_if<std::string>\(&([a-zA-Z0-9_\[\]\->]+)\)',
        r'\1.as_string()',
        text,
    )

    # holds_alternative -> is_*
    text = re.sub(
        r'std::holds_alternative<int64_t>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'\1.is_int()',
        text,
    )
    text = re.sub(
        r'std::holds_alternative<double>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'\1.is_double()',
        text,
    )
    text = re.sub(
        r'std::holds_alternative<std::string>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'\1.is_string()',
        text,
    )
    text = re.sub(
        r'std::holds_alternative<Keyword>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'\1.is_keyword()',
        text,
    )
    text = re.sub(
        r'std::holds_alternative<Symbol>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'\1.is_symbol()',
        text,
    )
    text = re.sub(
        r'std::holds_alternative<std::shared_ptr<AffineTransform>>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'\1.is_transform()',
        text,
    )

    # std::get<T>(v) -> v.accessor() for scalar-like T
    text = re.sub(
        r'std::get<int64_t>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'\1.int_val()',
        text,
    )
    text = re.sub(
        r'std::get<double>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'\1.double_val()',
        text,
    )
    text = re.sub(
        r'std::get<std::string>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'*\1.as_string()',
        text,
    )
    text = re.sub(
        r'std::get<bool>\(([a-zA-Z0-9_\[\]\->.]+)\)',
        r'\1.bool_val()',
        text,
    )

    # Simple single-line if-return for int/double get_if
    # if (const auto* i = std::get_if<int64_t>(&v)) return static_cast<double>(*i);
    text = re.sub(
        r'if \(const auto\* [a-zA-Z_][a-zA-Z0-9_]* = ([a-zA-Z_][a-zA-Z0-9_]*)\.as_string\(\)\) return \*\*?;',
        '',
        text,
    )  # placeholder, not used

    # Replace simple int/double get_if single-line returns
    text = re.sub(
        r'if \(const auto\* [a-zA-Z_][a-zA-Z0-9_]* = std::get_if<int64_t>\(&([a-zA-Z_][a-zA-Z0-9_]*)\)\) return static_cast<double>\(\*\1\);',
        r'if (\1.is_int()) return static_cast<double>(\1.int_val());',
        text,
    )
    text = re.sub(
        r'if \(const auto\* [a-zA-Z_][a-zA-Z0-9_]* = std::get_if<double>\(&([a-zA-Z_][a-zA-Z0-9_]*)\)\) return \*\1;',
        r'if (\1.is_double()) return \1.double_val();',
        text,
    )
    text = re.sub(
        r'if \(const auto\* [a-zA-Z_][a-zA-Z0-9_]* = std::get_if<int64_t>\(&([a-zA-Z_][a-zA-Z0-9_]*)\)\) return static_cast<int>\(\*\1\);',
        r'if (\1.is_int()) return static_cast<int>(\1.int_val());',
        text,
    )
    text = re.sub(
        r'if \(const auto\* [a-zA-Z_][a-zA-Z0-9_]* = std::get_if<double>\(&([a-zA-Z_][a-zA-Z0-9_]*)\)\) return static_cast<int>\(\*\1\);',
        r'if (\1.is_double()) return static_cast<int>(\1.double_val());',
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
