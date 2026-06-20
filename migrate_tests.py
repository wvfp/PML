import re
from pathlib import Path

ROOT = Path("g:/Project/PML/tests")
FILES = list(ROOT.glob("*.cpp")) + [ROOT / "test_helpers.h"]

def transform(text: str) -> str:
    # std::get<int64_t>(*r) / (*v) / (*result) / list->elements[i] etc.
    # For dereferenced Value pointers/results:  (*X).int_val()  or  X->int_val()
    # We handle the most common patterns.

    # std::get<int64_t>(*r) -> r->int_val()
    text = re.sub(r'std::get<int64_t>\(\*([a-zA-Z_][a-zA-Z0-9_]*)\)', r'\1->int_val()', text)
    # std::get<double>(*r) -> r->double_val()
    text = re.sub(r'std::get<double>\(\*([a-zA-Z_][a-zA-Z0-9_]*)\)', r'\1->double_val()', text)
    # std::get<std::string>(*r) -> *r->as_string()
    text = re.sub(r'std::get<std::string>\(\*([a-zA-Z_][a-zA-Z0-9_]*)\)', r'*\1->as_string()', text)
    # std::get<bool>(*r) -> r->bool_val()
    text = re.sub(r'std::get<bool>\(\*([a-zA-Z_][a-zA-Z0-9_]*)\)', r'\1->bool_val()', text)

    # std::get<int64_t>(vl->elements[i]) -> vl->elements[i].int_val()
    text = re.sub(
        r'std::get<int64_t>\(([a-zA-Z_][a-zA-Z0-9_]*->elements\[[^\]]+\])\)',
        r'\1.int_val()',
        text,
    )
    # std::get<double>(anim.from_value) -> anim.from_value.double_val()
    text = re.sub(
        r'std::get<double>\(([a-zA-Z_][a-zA-Z0-9_]*(?:\.[a-zA-Z_][a-zA-Z0-9_]*)+)\)',
        r'\1.double_val()',
        text,
    )
    # std::get<int64_t>(list->elements[i]) already handled above; catch other member access
    text = re.sub(
        r'std::get<int64_t>\(([a-zA-Z_][a-zA-Z0-9_]*(?:\.[a-zA-Z_][a-zA-Z0-9_]*)+)\)',
        r'\1.int_val()',
        text,
    )

    # std::get<std::shared_ptr<pml::ValueList>>(*r) -> *r->as_list()
    text = re.sub(
        r'std::get<std::shared_ptr<pml::ValueList>>\(\*([a-zA-Z_][a-zA-Z0-9_]*)\)',
        r'*\1->as_list()',
        text,
    )
    # std::get<pml::Symbol>(*r) -> *r->as_symbol()
    text = re.sub(
        r'std::get<pml::Symbol>\(\*([a-zA-Z_][a-zA-Z0-9_]*)\)',
        r'*\1->as_symbol()',
        text,
    )
    # std::get<pml::Symbol>(vl->elements[0]) -> *vl->elements[0].as_symbol()
    text = re.sub(
        r'std::get<pml::Symbol>\(([a-zA-Z_][a-zA-Z0-9_]*(?:\.[a-zA-Z_][a-zA-Z0-9_]*)*)\)',
        r'*\1.as_symbol()',
        text,
    )

    # std::holds_alternative<int64_t>(*v) -> v->is_int()
    text = re.sub(
        r'std::holds_alternative<int64_t>\(\*([a-zA-Z_][a-zA-Z0-9_]*)\)',
        r'\1->is_int()',
        text,
    )
    # std::holds_alternative<bool>(*r) -> r->is_bool()
    text = re.sub(
        r'std::holds_alternative<bool>\(\*([a-zA-Z_][a-zA-Z0-9_]*)\)',
        r'\1->is_bool()',
        text,
    )
    # std::holds_alternative<std::shared_ptr<pml::ValueList>>(*result) -> result->is_list()
    text = re.sub(
        r'std::holds_alternative<std::shared_ptr<pml::ValueList>>\(\*([a-zA-Z_][a-zA-Z0-9_]*)\)',
        r'\1->is_list()',
        text,
    )

    # std::get<std::shared_ptr<ValueList>>(exprs[0]) -> *exprs[0].as_list()
    text = re.sub(
        r'std::get<std::shared_ptr<ListExpr>>\(([a-zA-Z_][a-zA-Z0-9_]*(?:\[[^\]]+\])*)\)',
        r'*\1.as_list()',
        text,
    )
    # Parser tests use Expr, not Value; keep Expr access with std::get for now,
    # but we can convert the common forms to as_* helpers if desired.
    # The following are Expr patterns and should NOT be changed:
    # std::get<int64_t>(exprs[0]) where exprs is std::vector<Expr>
    # std::get<Symbol>(elems[0]) etc.
    # We leave them alone by not adding generic patterns.

    return text

for path in FILES:
    original = path.read_text(encoding="utf-8")
    changed = transform(original)
    if changed != original:
        path.write_text(changed, encoding="utf-8")
        print(f"updated {path.name}")
    else:
        print(f"no change {path.name}")
