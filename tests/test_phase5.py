"""Tests for Phase 5 — module system and macro system."""

import os
import tempfile
from pathlib import Path

import pytest

from pml.environment import Environment
from pml.errors import (
    AccessError,
    CircularImportError,
    ImportError_,
    MacroExpansionDepthError,
)
from pml.graphics.objects import GraphicObject
from pml.lexer import Lexer
from pml.parser import Parser
from pml.evaluator import evaluate
from pml.expander import Expander
from pml.module_loader import Module, ModuleLoader
from pml.repl import create_global_env
from pml.types import Macro, Symbol


def run(source: str, env: Environment | None = None) -> object:
    """Parse, expand, and evaluate PML source."""
    if env is None:
        env = create_global_env()
    tokens = Lexer(source).tokenize()
    ast = Parser(tokens).parse()
    expander = Expander(env)
    ast = expander.expand_all(ast)
    result = None
    for expr in ast:
        result = evaluate(expr, env)
    return result


# ======================================================================
# Module System
# ======================================================================


class TestModule:
    def test_module_get_exported(self):
        env = Environment()
        env.define("x", 42)
        env.exports.add("x")
        mod = Module("test", env, env.exports)
        assert mod.get("x") == 42

    def test_module_get_non_exported_raises(self):
        env = Environment()
        env.define("secret", 99)
        mod = Module("test", env, set())
        with pytest.raises(AccessError):
            mod.get("secret")

    def test_module_has(self):
        mod = Module("test", Environment(), {"a", "b"})
        assert mod.has("a")
        assert not mod.has("c")


class TestModuleLoader:
    def test_load_simple_module(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            # Write a module file
            mod_path = os.path.join(tmpdir, "mymod.pml")
            with open(mod_path, "w") as f:
                f.write('(define x 42)\n(provide x)\n')

            global_env = create_global_env()
            loader = ModuleLoader(global_env)
            module = loader.load(mod_path)

            assert isinstance(module, Module)
            assert module.get("x") == 42

    def test_module_caching(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            mod_path = os.path.join(tmpdir, "cached.pml")
            with open(mod_path, "w") as f:
                f.write('(define val 1)\n(provide val)\n')

            global_env = create_global_env()
            loader = ModuleLoader(global_env)
            m1 = loader.load(mod_path)
            m2 = loader.load(mod_path)
            assert m1 is m2  # Same object — cached

    def test_circular_import_detection(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            # Module A imports B, B imports A
            a_path = Path(tmpdir, "mod_a.pml")
            b_path = Path(tmpdir, "mod_b.pml")

            with open(a_path, "w") as f:
                f.write(f'(import "{b_path.as_posix()}" as b)\n(provide nothing)\n')
            with open(b_path, "w") as f:
                f.write(f'(import "{a_path.as_posix()}" as a)\n(provide nothing)\n')

            global_env = create_global_env()
            loader = ModuleLoader(global_env)
            with pytest.raises(CircularImportError):
                loader.load(str(a_path))

    def test_module_not_found(self):
        global_env = create_global_env()
        loader = ModuleLoader(global_env)
        with pytest.raises(ImportError_):
            loader.load("nonexistent.pml")

    def test_relative_path_resolution(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            lib_path = os.path.join(tmpdir, "lib.pml")
            main_path = os.path.join(tmpdir, "main.pml")

            with open(lib_path, "w") as f:
                f.write('(define pi 3.14)\n(provide pi)\n')

            global_env = create_global_env()
            loader = ModuleLoader(global_env)
            module = loader.load("lib.pml", from_file=main_path)
            assert module.get("pi") == 3.14


# ======================================================================
# Provide / Import special forms
# ======================================================================


class TestProvideImport:
    def test_provide_sets_exports(self):
        env = Environment()
        run('(provide x y z)', env)
        assert "x" in env.exports
        assert "y" in env.exports
        assert "z" in env.exports

    def test_import_loads_module(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            mod_path = Path(tmpdir, "greet.pml")
            with open(mod_path, "w") as f:
                f.write('(define greeting "hello")\n(provide greeting)\n')

            posix_path = mod_path.as_posix()
            env = create_global_env()
            # Use define to capture the value, since bare symbol access needs evaluation
            tokens = Lexer(f'(import "{posix_path}" as g)\n(define result g/greeting)\nresult').tokenize()
            ast = Parser(tokens).parse()
            expander = Expander(env)
            ast = expander.expand_all(ast)
            result = None
            for expr in ast:
                result = evaluate(expr, env)
            assert result == "hello"

    def test_import_with_default_prefix(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            mod_path = Path(tmpdir, "math_utils.pml")
            with open(mod_path, "w") as f:
                f.write('(define pi 3.14159)\n(provide pi)\n')

            posix_path = mod_path.as_posix()
            env = create_global_env()
            tokens = Lexer(f'(import "{posix_path}")\n(define result math_utils/pi)\nresult').tokenize()
            ast = Parser(tokens).parse()
            expander = Expander(env)
            ast = expander.expand_all(ast)
            result = None
            for expr in ast:
                result = evaluate(expr, env)
            assert abs(result - 3.14159) < 0.001

    def test_access_non_exported_raises(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            mod_path = Path(tmpdir, "secret.pml")
            with open(mod_path, "w") as f:
                f.write('(define hidden 42)\n(define visible 1)\n(provide visible)\n')

            posix_path = mod_path.as_posix()
            env = create_global_env()
            with pytest.raises(AccessError):
                tokens = Lexer(f'(import "{posix_path}" as s)\n(s/hidden)').tokenize()
                ast = Parser(tokens).parse()
                expander = Expander(env)
                ast = expander.expand_all(ast)
                for expr in ast:
                    evaluate(expr, env)


# ======================================================================
# Macro System
# ======================================================================


class TestMacro:
    def test_simple_macro(self):
        result = run('(defmacro double (x) (+ x x))\n(double 5)')
        assert result == 10

    def test_macro_with_multiple_params(self):
        result = run('(defmacro my-add (a b) (+ a b))\n(my-add 3 7)')
        assert result == 10

    def test_macro_generates_code(self):
        result = run('''
            (defmacro square (x) (* x x))
            (square 4)
        ''')
        assert result == 16

    def test_macro_rest_params(self):
        """Macro with rest parameter collects extra args."""
        result = run('''
            (defmacro first-of (. args) (car args))
            (first-of 1 2 3)
        ''')
        assert result == 1

    def test_macro_with_define(self):
        result = run('''
            (defmacro def-pair (name val)
                (define name val))
            (def-pair x 42)
            x
        ''')
        assert result == 42

    def test_nested_macro_expansion(self):
        """Macros that produce other macro calls get re-expanded."""
        result = run('''
            (defmacro double (x) (+ x x))
            (defmacro quadruple (x) (double (double x)))
            (quadruple 3)
        ''')
        assert result == 12

    def test_macro_depth_limit(self):
        """Infinite recursive macro should hit depth limit."""
        with pytest.raises(MacroExpansionDepthError):
            run('''
                (defmacro loop () (loop))
                (loop)
            ''')

    def test_macro_with_gensym(self):
        """Macros can use gensym for hygiene."""
        result = run('''
            (defmacro swap (a b)
                (let ((tmp (gensym "tmp")))
                    (define tmp a)
                    (set! a b)
                    (set! b tmp)))
            (define x 1)
            (define y 2)
            (swap x y)
            x
        ''')
        assert result == 2

    def test_macro_does_not_affect_quote(self):
        """Quoted expressions should not be expanded."""
        result = run('''
            (defmacro m () 42)
            (quote (m))
        ''')
        assert isinstance(result, list)
        # The quoted (m) should remain as-is
        assert len(result) == 1


class TestExpander:
    def test_expander_preserves_atoms(self):
        env = create_global_env()
        expander = Expander(env)
        assert expander.expand(42) == 42
        assert expander.expand("hello") == "hello"
        assert expander.expand(True) is True

    def test_expander_preserves_empty_list(self):
        env = create_global_env()
        expander = Expander(env)
        assert expander.expand([]) == []

    def test_expander_recursive(self):
        """Expander should recursively expand sub-expressions."""
        result = run('''
            (defmacro inc (x) (+ x 1))
            (+ (inc 2) (inc 3))
        ''')
        assert result == 7  # (2+1) + (3+1) = 7

    def test_expand_all(self):
        env = create_global_env()
        tokens = Lexer('(defmacro d (x) (+ x 1))\n(d 5)\n(d 10)').tokenize()
        ast = Parser(tokens).parse()
        expander = Expander(env)

        # First expand defmacro to register it
        result = None
        expanded = expander.expand_all(ast)
        for expr in expanded:
            result = evaluate(expr, env)
        assert result == 11


# ======================================================================
# Integration: macros + modules together
# ======================================================================


class TestIntegration:
    def test_macro_in_module(self):
        """Module can define macros that work after import."""
        with tempfile.TemporaryDirectory() as tmpdir:
            mod_path = Path(tmpdir, "macros.pml")
            with open(mod_path, "w") as f:
                f.write('(defmacro triple (x) (+ x x x))\n(provide triple)\n')

            posix_path = mod_path.as_posix()
            env = create_global_env()
            # Import and use the macro — evaluator now handles macros inline
            code = f'(import "{posix_path}" as m)\n(m/triple 5)'
            tokens = Lexer(code).tokenize()
            ast = Parser(tokens).parse()
            result = None
            for expr in ast:
                result = evaluate(expr, env)
            assert result == 15

    def test_existing_tests_still_pass(self):
        """Verify existing evaluator features still work with expander in pipeline."""
        assert run("(+ 1 2)") == 3
        assert run("(if #t 'yes 'no)") == Symbol("yes")
        assert run("(let ((x 10)) (+ x 1))") == 11
        assert run("(map (lambda (x) (* x 2)) (list 1 2 3))") == [2, 4, 6]

    def test_full_pml_file_with_macros(self):
        """Test run_file with a file containing macros."""
        with tempfile.TemporaryDirectory() as tmpdir:
            pml_path = Path(tmpdir, "test.pml")
            with open(pml_path, "w") as f:
                f.write('(defmacro twice (x) (+ x x))\n(twice 21)\n')

            from pml.repl import run_file
            result = run_file(str(pml_path))
            assert result == 42
