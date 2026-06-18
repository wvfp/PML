"""PML evaluator — tree-walking interpreter with special forms."""

from __future__ import annotations

from typing import Any

from pml.environment import Environment
from pml.errors import (
    ArityError,
    MacroExpansionDepthError,
    PMLError,
    PMLTypeError,
)
from pml.types import BuiltinProcedure, Expr, Keyword, Macro, Procedure, Symbol


# Module-level macro expansion depth counter.
# Incremented when entering a macro expansion, decremented on exit.
# Avoids threading a parameter through every evaluate() call site.
_macro_depth: int = 0
_MAX_MACRO_DEPTH: int = 256


# ======================================================================
# Main evaluate function
# ======================================================================


def evaluate(expr: Expr, env: Environment) -> Any:
    """Evaluate a PML expression in the given environment."""
    # Self-evaluating atoms
    if isinstance(expr, (int, float, bool)) or expr is None:
        return expr
    if isinstance(expr, str):
        return expr

    # Symbol lookup
    if isinstance(expr, Symbol):
        name = expr.name
        # Handle module access: prefix/symbol
        if "/" in name:
            parts = name.split("/", 1)
            prefix_val = env.try_lookup(parts[0])
            if prefix_val is not None:
                from pml.module_loader import Module
                if isinstance(prefix_val, Module):
                    return prefix_val.get(parts[1])
        return env.lookup(name)

    # Keyword — return as-is (used as parameter markers)
    if isinstance(expr, Keyword):
        return expr

    # List — special form or function call
    if isinstance(expr, list):
        if len(expr) == 0:
            return []

        head = expr[0]

        # Special forms
        if isinstance(head, Symbol) and head.name in SPECIAL_FORMS:
            return SPECIAL_FORMS[head.name](expr, env)

        # Macro expansion: if head resolves to a Macro, expand and re-evaluate
        if isinstance(head, Symbol):
            macro_val = env.try_lookup(head.name)
            if isinstance(macro_val, Macro):
                return _expand_macro(macro_val, expr[1:], env)

        # Regular function call
        func = evaluate(head, env)

        # Check if evaluated function is a Macro (for higher-order macro use)
        if isinstance(func, Macro):
            return _expand_macro(func, expr[1:], env)

        args, kwargs = evaluate_arguments(expr[1:], env)
        return apply_function(func, args, kwargs)

    raise PMLTypeError(f"Cannot evaluate: {expr!r}")


def _expand_macro(macro: Macro, args: list[Expr], env: Environment) -> Any:
    """Expand a macro and evaluate the result, with depth tracking."""
    global _macro_depth
    _macro_depth += 1
    if _macro_depth > _MAX_MACRO_DEPTH:
        _macro_depth = 0
        raise MacroExpansionDepthError(
            f"Macro '{macro.name}' expansion exceeded depth limit of {_MAX_MACRO_DEPTH}"
        )
    try:
        expanded = macro.expand(args)
        return evaluate(expanded, env)
    finally:
        _macro_depth -= 1


# ======================================================================
# Argument evaluation
# ======================================================================


def evaluate_arguments(
    exprs: list[Expr], env: Environment
) -> tuple[list[Any], dict[str, Any]]:
    """Evaluate argument expressions, separating positional and keyword args."""
    args: list[Any] = []
    kwargs: dict[str, Any] = {}
    i = 0
    while i < len(exprs):
        expr = exprs[i]
        if isinstance(expr, Keyword):
            if i + 1 >= len(exprs):
                raise PMLTypeError(f"Keyword {expr} has no following value")
            kwargs[expr.name] = evaluate(exprs[i + 1], env)
            i += 2
        else:
            args.append(evaluate(expr, env))
            i += 1
    return args, kwargs


# ======================================================================
# Function application
# ======================================================================


def apply_function(
    func: Any, args: list[Any], kwargs: dict[str, Any]
) -> Any:
    """Apply a function to evaluated arguments."""
    if isinstance(func, Procedure):
        return _apply_procedure(func, args)
    if isinstance(func, BuiltinProcedure):
        return _apply_builtin(func, args, kwargs)
    if callable(func):
        return func(*args)
    raise PMLTypeError(f"Not a procedure: {func!r}")


def _apply_procedure(proc: Procedure, args: list[Any]) -> Any:
    """Call a user-defined procedure with positional args."""
    # Handle rest parameters (last param starts with '.')
    # For now, simple parameter matching
    expected = len(proc.params)
    got = len(args)

    # Check for variadic parameter (convention: param name starts with '...')
    if proc.params and proc.params[-1] == ".":
        # (lambda (fmt . args) ...) — not yet fully implemented
        if got < expected - 2:
            raise ArityError(
                f"Expected at least {expected - 2} arguments, got {got}"
            )
    elif expected != got:
        raise ArityError(
            f"Expected {expected} arguments, got {got}"
        )

    call_env = proc.closure_env.extend(proc.params, args)
    result = None
    for body_expr in proc.body:
        result = evaluate(body_expr, call_env)
    return result


def _apply_builtin(
    func: BuiltinProcedure, args: list[Any], kwargs: dict[str, Any]
) -> Any:
    """Call a built-in Python function."""
    if func.accepts_kwargs:
        return func.fn(*args, **kwargs)
    if kwargs:
        raise PMLTypeError(
            f"{func.name} does not accept keyword arguments"
        )
    return func.fn(*args)


# ======================================================================
# Truthiness
# ======================================================================


def is_truthy(value: Any) -> bool:
    """PML truthiness: everything is truthy except #f, 0, None, and empty list."""
    if value is False or value is None:
        return False
    if value == 0:
        return False
    if isinstance(value, list) and len(value) == 0:
        return False
    return True


# ======================================================================
# Special forms
# ======================================================================

SPECIAL_FORMS: dict[str, Any] = {}


def _eval_quote(expr: Expr, env: Environment) -> Any:
    """(quote <expr>) → return expr unevaluated."""
    if len(expr) != 2:
        raise ArityError("quote expects exactly 1 argument")
    return expr[1]


SPECIAL_FORMS["quote"] = _eval_quote


def _eval_if(expr: Expr, env: Environment) -> Any:
    """(if <cond> <then> <else>)"""
    if len(expr) < 3 or len(expr) > 4:
        raise ArityError("if expects 2 or 3 arguments")
    condition = evaluate(expr[1], env)
    if is_truthy(condition):
        return evaluate(expr[2], env)
    elif len(expr) == 4:
        return evaluate(expr[3], env)
    return None


SPECIAL_FORMS["if"] = _eval_if


def _eval_cond(expr: Expr, env: Environment) -> Any:
    """(cond (<test> <expr>) ... (else <default>))"""
    for clause in expr[1:]:
        if not isinstance(clause, list) or len(clause) < 2:
            raise PMLTypeError("cond clause must be (test expr)")
        test = clause[0]
        # Check for else
        if isinstance(test, Symbol) and test.name == "else":
            result = None
            for e in clause[1:]:
                result = evaluate(e, env)
            return result
        if is_truthy(evaluate(test, env)):
            result = None
            for e in clause[1:]:
                result = evaluate(e, env)
            return result
    return None


SPECIAL_FORMS["cond"] = _eval_cond


def _eval_define(expr: Expr, env: Environment) -> Any:
    """(define <name> <expr>) or (define (<name> <params>) <body>)"""
    if len(expr) < 3:
        raise ArityError("define expects at least 2 arguments")

    target = expr[1]

    # Function shorthand: (define (name params...) body...)
    if isinstance(target, list):
        if len(target) == 0:
            raise PMLTypeError("define function form requires a name")
        name = target[0]
        if not isinstance(name, Symbol):
            raise PMLTypeError(f"define: expected symbol, got {name!r}")
        params = []
        for p in target[1:]:
            if not isinstance(p, Symbol):
                raise PMLTypeError(f"define: parameter must be symbol, got {p!r}")
            params.append(p.name)
        proc = Procedure(params, expr[2:], env, name=name.name)
        env.define(name.name, proc)
        return None

    # Variable: (define name expr)
    if not isinstance(target, Symbol):
        raise PMLTypeError(f"define: expected symbol, got {target!r}")
    value = evaluate(expr[2], env)
    env.define(target.name, value)
    return None


SPECIAL_FORMS["define"] = _eval_define


def _eval_lambda(expr: Expr, env: Environment) -> Procedure:
    """(lambda (<params>) <body>)"""
    if len(expr) < 3:
        raise ArityError("lambda expects parameter list and body")
    params_expr = expr[1]
    if not isinstance(params_expr, list):
        raise PMLTypeError("lambda: parameters must be a list")
    params = []
    for p in params_expr:
        if not isinstance(p, Symbol):
            raise PMLTypeError(f"lambda: parameter must be symbol, got {p!r}")
        params.append(p.name)
    return Procedure(params, expr[2:], env)


SPECIAL_FORMS["lambda"] = _eval_lambda


def _eval_let(expr: Expr, env: Environment) -> Any:
    """(let ((name expr) ...) body) — parallel bindings."""
    if len(expr) < 3:
        raise ArityError("let expects bindings and body")
    bindings_expr = expr[1]
    if not isinstance(bindings_expr, list):
        raise PMLTypeError("let: bindings must be a list")

    names: list[str] = []
    values: list[Any] = []
    for binding in bindings_expr:
        if not isinstance(binding, list) or len(binding) != 2:
            raise PMLTypeError("let: each binding must be (name expr)")
        name = binding[0]
        if not isinstance(name, Symbol):
            raise PMLTypeError(f"let: binding name must be symbol, got {name!r}")
        names.append(name.name)
        # All values are evaluated in the OUTER environment (parallel)
        values.append(evaluate(binding[1], env))

    child_env = env.extend(names, values)
    result = None
    for body_expr in expr[2:]:
        result = evaluate(body_expr, child_env)
    return result


SPECIAL_FORMS["let"] = _eval_let


def _eval_let_star(expr: Expr, env: Environment) -> Any:
    """(let* ((name expr) ...) body) — sequential bindings."""
    if len(expr) < 3:
        raise ArityError("let* expects bindings and body")
    bindings_expr = expr[1]
    if not isinstance(bindings_expr, list):
        raise PMLTypeError("let*: bindings must be a list")

    # Create child environment and bind sequentially
    child_env = Environment(parent=env)
    for binding in bindings_expr:
        if not isinstance(binding, list) or len(binding) != 2:
            raise PMLTypeError("let*: each binding must be (name expr)")
        name = binding[0]
        if not isinstance(name, Symbol):
            raise PMLTypeError(f"let*: binding name must be symbol, got {name!r}")
        # Each value is evaluated in the growing child environment
        value = evaluate(binding[1], child_env)
        child_env.define(name.name, value)

    result = None
    for body_expr in expr[2:]:
        result = evaluate(body_expr, child_env)
    return result


SPECIAL_FORMS["let*"] = _eval_let_star


def _eval_begin(expr: Expr, env: Environment) -> Any:
    """(begin <expr> ...) — evaluate sequentially, return last."""
    result = None
    for sub_expr in expr[1:]:
        result = evaluate(sub_expr, env)
    return result


SPECIAL_FORMS["begin"] = _eval_begin


def _eval_set(expr: Expr, env: Environment) -> Any:
    """(set! <name> <expr>)"""
    if len(expr) != 3:
        raise ArityError("set! expects exactly 2 arguments")
    name = expr[1]
    if not isinstance(name, Symbol):
        raise PMLTypeError(f"set!: expected symbol, got {name!r}")
    value = evaluate(expr[2], env)
    env.set(name.name, value)
    return None


SPECIAL_FORMS["set!"] = _eval_set


def _eval_and(expr: Expr, env: Environment) -> Any:
    """(and <expr> ...) — short-circuit, return last truthy or first falsy."""
    result: Any = True
    for sub_expr in expr[1:]:
        result = evaluate(sub_expr, env)
        if not is_truthy(result):
            return result
    return result


SPECIAL_FORMS["and"] = _eval_and


def _eval_or(expr: Expr, env: Environment) -> Any:
    """(or <expr> ...) — short-circuit, return first truthy."""
    for sub_expr in expr[1:]:
        result = evaluate(sub_expr, env)
        if is_truthy(result):
            return result
    return False


SPECIAL_FORMS["or"] = _eval_or


def _eval_do(expr: Expr, env: Environment) -> Any:
    """(do ((var init step) ...) (test result) body...)"""
    if len(expr) < 3:
        raise ArityError("do expects variable clauses, test clause, and body")

    # Parse variable clauses
    var_clauses = expr[1]
    if not isinstance(var_clauses, list):
        raise PMLTypeError("do: variable clauses must be a list")

    test_clause = expr[2]
    if not isinstance(test_clause, list) or len(test_clause) < 1:
        raise PMLTypeError("do: test clause must be (test [result])")

    body = expr[3:]

    # Initialize variables
    names: list[str] = []
    inits: list[Expr] = []
    steps: list[Expr] = []
    for clause in var_clauses:
        if not isinstance(clause, list) or len(clause) < 2:
            raise PMLTypeError("do: each variable clause must be (var init [step])")
        name = clause[0]
        if not isinstance(name, Symbol):
            raise PMLTypeError(f"do: variable must be symbol, got {name!r}")
        names.append(name.name)
        inits.append(clause[1])
        steps.append(clause[2] if len(clause) > 2 else name)

    # Create loop environment
    loop_env = Environment(parent=env)
    for name, init_expr in zip(names, inits):
        loop_env.define(name, evaluate(init_expr, env))

    # Loop
    while True:
        # Check test
        test_result = evaluate(test_clause[0], loop_env)
        if is_truthy(test_result):
            # Return result expression (or None)
            if len(test_clause) > 1:
                return evaluate(test_clause[1], loop_env)
            return None

        # Execute body
        for body_expr in body:
            evaluate(body_expr, loop_env)

        # Update variables (evaluate steps in current env, then assign)
        new_values = [evaluate(step, loop_env) for step in steps]
        for name, value in zip(names, new_values):
            loop_env.set(name, value)


SPECIAL_FORMS["do"] = _eval_do


def _eval_quasiquote(expr: Expr, env: Environment) -> Any:
    """(quasiquote <expr>) — template with unquote interpolation."""
    if len(expr) != 2:
        raise ArityError("quasiquote expects exactly 1 argument")
    return _expand_quasiquote(expr[1], env)


def _expand_quasiquote(template: Any, env: Environment) -> Any:
    """Recursively expand a quasiquote template."""
    if not isinstance(template, list):
        return template

    if len(template) > 0 and isinstance(template[0], Symbol):
        if template[0].name == "unquote" and len(template) == 2:
            return evaluate(template[1], env)
        if template[0].name == "unquote-splicing" and len(template) == 2:
            raise PMLTypeError("unquote-splicing used outside of list context")

    result: list[Any] = []
    for item in template:
        if isinstance(item, list) and len(item) > 0 and isinstance(item[0], Symbol):
            if item[0].name == "unquote-splicing" and len(item) == 2:
                spliced = evaluate(item[1], env)
                if not isinstance(spliced, list):
                    raise PMLTypeError("unquote-splicing: value must be a list")
                result.extend(spliced)
                continue
        result.append(_expand_quasiquote(item, env))
    return result


SPECIAL_FORMS["quasiquote"] = _eval_quasiquote


# ======================================================================
# Module system special forms
# ======================================================================


def _eval_provide(expr: Expr, env: Environment) -> None:
    """(provide sym1 sym2 ...) — declare exported symbols from this module."""
    for sym_expr in expr[1:]:
        if not isinstance(sym_expr, Symbol):
            raise PMLTypeError(f"provide: expected symbol, got {sym_expr!r}")
        env.exports.add(sym_expr.name)
    return None


def _eval_provide_check(expr: Expr, env: Environment) -> None:
    """(provide-check) — report mismatches between defines and provides.

    Scans the current module's direct bindings and compares against
    its export set. Prints warnings for:
      - Defined but not exported (forgotten provide)
      - Exported but not defined (typo in provide)

    Ignores symbols starting with _ (internal) and special form names.
    """
    SPECIAL_FORM_NAMES = {
        "define", "set!", "if", "cond", "and", "or", "lambda",
        "let", "let*", "quote", "quasiquote", "unquote",
        "do", "defmacro", "import", "provide", "provide-check",
    }
    direct: set[str] = set(env.bindings.keys())
    exports: set[str] = set(env.exports)

    # Defined but not exported
    missing = direct - exports - SPECIAL_FORM_NAMES - {"_", "..."}
    missing = {s for s in missing if not s.startswith("_") and not s.startswith(":")}

    # Exported but not defined
    extra = exports - direct - SPECIAL_FORM_NAMES

    if not missing and not extra:
        return

    import sys
    print(";; ── provide-check ──", file=sys.stderr)
    for s in sorted(missing):
        print(f";;   ⚠ defined but NOT exported: {s}", file=sys.stderr)
    for s in sorted(extra):
        print(f";;   ⚠ exported but NOT defined: {s}", file=sys.stderr)
    print(";; ───────────────────", file=sys.stderr)
    return None


SPECIAL_FORMS["provide"] = _eval_provide
SPECIAL_FORMS["provide-check"] = _eval_provide_check


def _eval_import(expr: Expr, env: Environment) -> None:
    """(import "path.pml" as prefix) — load a module and bind it to a prefix."""
    if len(expr) < 2:
        raise ArityError("import expects at least a path")

    path_expr = expr[1]
    if not isinstance(path_expr, str):
        raise PMLTypeError(f"import: path must be a string, got {path_expr!r}")

    # Parse optional 'as prefix' clause
    prefix = None
    if len(expr) >= 4:
        as_kw = expr[2]
        if isinstance(as_kw, Symbol) and as_kw.name == "as":
            prefix_sym = expr[3]
            if not isinstance(prefix_sym, Symbol):
                raise PMLTypeError(f"import: prefix must be a symbol, got {prefix_sym!r}")
            prefix = prefix_sym.name
    elif len(expr) == 3:
        # (import "path" prefix) — without 'as' keyword
        prefix_sym = expr[2]
        if isinstance(prefix_sym, Symbol):
            prefix = prefix_sym.name

    # Default prefix from filename
    if prefix is None:
        from pathlib import Path
        prefix = Path(path_expr).stem

    # Get or create the module loader
    loader = _get_module_loader(env)

    # Determine the importing file's path for relative resolution
    from_file_val = env.try_lookup("__source_file__")
    from_file = from_file_val if isinstance(from_file_val, str) else ""

    # Load the module
    module = loader.load(path_expr, from_file)

    # Bind the module object to the prefix name
    env.define(prefix, module)
    return None


SPECIAL_FORMS["import"] = _eval_import


def _get_module_loader(env: Environment) -> Any:
    """Get the ModuleLoader instance, creating one if needed."""
    from pml.module_loader import ModuleLoader

    # Walk up to global env to find existing loader
    current: Environment | None = env
    while current is not None:
        loader = current.try_lookup("__module_loader__")
        if loader is not None and isinstance(loader, ModuleLoader):
            return loader
        current = current.parent

    # Create a new loader rooted at global env
    global_env = env
    while global_env.parent is not None:
        global_env = global_env.parent
    loader = ModuleLoader(global_env)

    # Store it in the global env for reuse
    global_env.define("__module_loader__", loader)
    return loader


# ======================================================================
# Macro system special form
# ======================================================================


def _eval_defmacro(expr: Expr, env: Environment) -> None:
    """(defmacro name (params) body...) — define a macro."""
    if len(expr) < 4:
        raise ArityError("defmacro expects name, parameter list, and body")

    name_expr = expr[1]
    if not isinstance(name_expr, Symbol):
        raise PMLTypeError(f"defmacro: name must be a symbol, got {name_expr!r}")

    params_expr = expr[2]
    if not isinstance(params_expr, list):
        raise PMLTypeError("defmacro: parameters must be a list")

    # Parse parameters, supporting rest param via '.' separator
    params: list[str] = []
    rest_param: str | None = None
    saw_dot = False

    for i, p in enumerate(params_expr):
        if isinstance(p, Symbol) and p.name == ".":
            saw_dot = True
            continue
        if not isinstance(p, Symbol):
            raise PMLTypeError(f"defmacro: parameter must be symbol, got {p!r}")
        if saw_dot:
            rest_param = p.name
        else:
            params.append(p.name)

    body = expr[3:]
    macro = Macro(
        name=name_expr.name,
        params=params,
        rest_param=rest_param,
        body=body,
        closure_env=env,
    )
    env.define(name_expr.name, macro)
    return None


SPECIAL_FORMS["defmacro"] = _eval_defmacro
