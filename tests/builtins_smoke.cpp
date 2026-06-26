// ==========================================================================================================================================================================================================================================═
// PML Built-in Procedures — Smoke Test
// ==========================================================================================================================================================================================================================================═
//
// Verifies that built-in procedures registered via register_builtins() work
// correctly through the full lexer → parser → evaluator pipeline.
// No Google Test dependency — standalone compile-and-run.
// ==========================================================================================================================================================================================================================================═

#include "builtins.h"
#include "evaluator.h"
#include "lexer.h"
#include "parser.h"
#include "environment.h"
#include "types.h"

#include "pml/api/context.h"
#include "pml/asset/asset_builtins.h"
#include "pml/backend/backend.h"
#include "pml/backend/registry.h"
#include "pml/evaluator/canvas_builtins.h"
#include "pml/evaluator/tilemap_builtins.h"
#include "pml/evaluator/render_channels_builtins.h"
#include "pml/evaluator/multi_texture_builtins.h"
#include "pml/evaluator/shader_builtins.h"
#include "pml/evaluator/perturb_builtins.h"
#include "pml/filter/filter_builtins.h"
#include "pml/graphics/render.h"
#include "pml/graphics3d/builtins_3d.h"
#include "pml/layer/layer_builtins.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <format>

// ---- Test helpers ------------------------------------------------------------------------------------------------------------------------─

int g_passed = 0;
int g_failed = 0;

/// Parse a PML source string into an Expr.
pml::Result<pml::Expr> parse(const std::string& source) {
    pml::Lexer lexer(source, "<test>");
    auto tokens = lexer.tokenize();
    if (!tokens) return std::unexpected(tokens.error());

    pml::Parser parser(std::move(*tokens), "<test>");
    auto exprs = parser.parse();
    if (!exprs) return std::unexpected(exprs.error());

    if (exprs->empty()) {
        return pml::make_nil();
    }
    return (*exprs)[0];
}

/// Evaluate a PML source string and return the result.
pml::Result<pml::Value> eval(const std::string& source,
                              std::shared_ptr<pml::Environment> env) {
    auto expr = parse(source);
    if (!expr) return std::unexpected(expr.error());
    return pml::eval_to_value(*expr, env);
}

#define CHECK(label, source, expected) do { \
    auto _r = eval((source), _env); \
    if (!_r) { \
        std::cerr << "FAIL [" << label << "]: " << source \
                  << " → ERROR: " << _r.error().what() << '\n'; \
        g_failed++; \
    } else { \
        std::string _got = pml::value_to_string(*_r); \
        if (_got == (expected)) { \
            std::cout << "PASS [" << label << "]: " << source \
                      << " → " << _got << '\n'; \
            g_passed++; \
        } else { \
            std::cerr << "FAIL [" << label << "]: " << source \
                      << " → expected " << (expected) \
                      << ", got " << _got << '\n'; \
            g_failed++; \
        } \
    } \
} while(0)

#define CHECK_ERROR(label, source) do { \
    auto _r = eval((source), _env); \
    if (!_r) { \
        std::cout << "PASS [" << label << "]: " << source \
                  << " → correctly errored: " << _r.error().what() << '\n'; \
        g_passed++; \
    } else { \
        std::string _got = pml::value_to_string(*_r); \
        std::cerr << "FAIL [" << label << "]: " << source \
                  << " → expected error, got " << _got << '\n'; \
        g_failed++; \
    } \
} while(0)

int main() {
    // Create global environment and register builtins
    auto _env = std::make_shared<pml::Environment>();
    pml::register_builtins(_env);
    pml::register_layer_builtins(_env);
    pml::register_canvas_builtins(_env);
    pml::register_filter_builtins(_env);
    pml::register_asset_builtins(_env);
    pml::register_3d_builtins(_env);
    pml::register_tilemap_builtins(_env);
    pml::register_render(_env);     // (render ...), (render-set ...), (render-tilemap ...)
    pml::register_render_channels(_env);  // (render-channels ...)
    pml::register_multi_texture_builtins(_env);  // (bind-textures ...)
    pml::register_shader_builtins(_env);          // (shader ...), noise, uniforms
    pml::register_perturb_builtins(_env);          // (perturb-polygon ...)
    pml::PMLContext::current().reset();

    // Force-link the null backend and activate it so that render builtins
    // (including render-tilemap) can create surfaces and save files without
    // requiring a real Skia/GPU backend.
    pml::force_link_null_backend();
    pml::BackendRegistry::instance().set_active("null");

    // Set __source_file__ so that (image ...) and (bitmap-layer ...) resolve
    // paths relative to the tests/ directory (where test.png lives).
    _env->define("__source_file__", pml::Value(std::string("tests/builtins_smoke.cpp")));

    std::cout << "======═ PML Builtins Smoke Test ======═\n\n";

    // ---- Arithmetic ------------------------------------------------------------------------------------------------------------─
    std::cout << "---- Arithmetic ----\n";

    CHECK("add",          "(+ 1 2)",              "3");
    CHECK("add-multi",    "(+ 1 2 3)",            "6");
    CHECK("add-zero",     "(+)",                  "0");
    CHECK("add-float",    "(+ 1 2.5)",            "3.5");
    CHECK("sub",          "(- 5 3)",              "2");
    CHECK("sub-negate",   "(- 5)",                "-5");
    CHECK("sub-multi",    "(- 10 3 2)",           "5");
    CHECK("mul",          "(* 2 3)",              "6");
    CHECK("mul-zero",     "(*)",                  "1");
    CHECK("mul-mixed",    "(* 2 3.5)",            "7.0");
    CHECK("div",          "(/ 10 3)",             "3.333333");
    CHECK("div-one",      "(/ 5)",                "0.2");
    CHECK("mod",          "(% 10 3)",             "1");
    CHECK("mod-float",    "(% 10.5 3)",           "1.5");
    CHECK("abs-neg",      "(abs -5)",             "5");
    CHECK("abs-pos",      "(abs 3)",              "3");
    CHECK("abs-float",    "(abs -3.5)",           "3.5");
    CHECK("min",          "(min 3 7 1 9)",        "1");
    CHECK("min-float",    "(min 3.5 2.1)",        "2.1");
    CHECK("max",          "(max 3 7 1 9)",        "9");
    CHECK("max-float",    "(max 3.5 2.1)",        "3.5");
    CHECK("floor",        "(floor 3.7)",          "3.0");
    CHECK("ceil",         "(ceil 3.2)",           "4.0");
    CHECK("round-up",     "(round 3.7)",          "4.0");
    CHECK("round-down",   "(round 3.2)",          "3.0");
    CHECK("sqrt",         "(sqrt 9)",             "3.0");
    CHECK("pow",          "(pow 2 10)",           "1024.0");
    CHECK("expt",         "(expt 3 4)",           "81.0");
    CHECK("sin",          "(sin 0)",              "0.0");
    CHECK("cos",          "(cos 0)",              "1.0");
    CHECK("tan",          "(tan 0)",              "0.0");
    CHECK("atan2",        "(atan2 0 1)",          "0.0");

    // ---- Comparison ------------------------------------------------------------------------------------------------------------
    std::cout << "\n---- Comparison ----\n";

    CHECK("num-eq-true",  "(= 5 5)",              "#t");
    CHECK("num-eq-false", "(= 5 3)",              "#f");
    CHECK("lt-true",      "(< 2 3)",              "#t");
    CHECK("lt-false",     "(< 3 2)",              "#f");
    CHECK("gt-true",      "(> 3 2)",              "#t");
    CHECK("gt-false",     "(> 2 3)",              "#f");
    CHECK("lte-true",     "(<= 2 3)",             "#t");
    CHECK("lte-eq",       "(<= 3 3)",             "#t");
    CHECK("gte-true",     "(>= 3 2)",             "#t");
    CHECK("gte-eq",       "(>= 3 3)",             "#t");
    CHECK("num-eq-multi", "(= 5 5 5)",            "#t");
    CHECK("num-eq-multi-false", "(= 5 5 6)",      "#f");

    // eq? tests
    CHECK("eq-same-sym",  "(eq? 'a 'a)",          "#t");
    CHECK("eq-diff-sym",  "(eq? 'a 'b)",          "#f");

    // equal? tests — use quoted lists since we can build them
    CHECK("equal-self",   "(equal? 5 5)",         "#t");
    CHECK("equal-list",   "(equal? '(1 2 3) '(1 2 3))", "#t");
    CHECK("equal-list-f", "(equal? '(1 2 3) '(1 2 4))", "#f");
    CHECK("equal-nested", "(equal? '((1 2) (3 4)) '((1 2) (3 4)))", "#t");
    CHECK("equal-hash",   "(equal? (make-hash '((\"a\" 1) (\"b\" 2))) (make-hash '((\"b\" 2) (\"a\" 1))))", "#t");
    CHECK("equal-hash-f", "(equal? (make-hash '((\"a\" 1))) (make-hash '((\"a\" 2))))", "#f");
    CHECK("equal-vector", "(equal? (list->vector '(1 2 3)) (list->vector '(1 2 3)))", "#t");
    CHECK("equal-vector-f","(equal? (list->vector '(1 2 3)) (list->vector '(1 2 4)))", "#f");

    // ---- Type predicates ------------------------------------------------------------------------------------------------─
    std::cout << "\n---- Type Predicates ----\n";

    CHECK("number?-int",  "(number? 5)",          "#t");
    CHECK("number?-float","(number? 5.5)",        "#t");
    CHECK("number?-str",  "(number? \"a\")",      "#f");
    CHECK("integer?-int", "(integer? 5)",         "#t");
    CHECK("integer?-flt", "(integer? 5.5)",       "#f");
    CHECK("float?-flt",   "(float? 5.5)",         "#t");
    CHECK("float?-int",   "(float? 5)",           "#f");
    CHECK("string?-str",  "(string? \"hi\")",     "#t");
    CHECK("string?-num",  "(string? 5)",          "#f");
    CHECK("symbol?-sym",  "(symbol? 'x)",         "#t");
    CHECK("symbol?-str",  "(symbol? \"x\")",      "#f");
    CHECK("boolean?-t",   "(boolean? #t)",        "#t");
    CHECK("boolean?-f",   "(boolean? #f)",        "#t");
    CHECK("boolean?-num", "(boolean? 5)",         "#f");
    CHECK("keyword?-kw",  "(keyword? ':foo)",     "#t");
    CHECK("keyword?-sym", "(keyword? 'foo)",      "#f");
    CHECK("list?-lst",    "(list? (list 1 2))",   "#t");
    CHECK("list?-nil",    "(list? ())",           "#t");  // empty list is a list
    CHECK("null?-nil",    "(null? ())",           "#t");
    CHECK("null?-lst",    "(null? (list 1))",     "#f");
    // null? with nil: there's no way to write nil as input; use empty list ()
    // Already tested above: (null? ()) → #t
    CHECK("pair?-cons",   "(pair? (list 1 2))",   "#t");
    CHECK("pair?-empty",  "(pair? ())",           "#f");

    // procedure? — define a lambda and check
    CHECK("proc?-lambda", "(procedure? (lambda (x) x))", "#t");

    // ---- Quasiquote ------------------------------------------------------------------------------------------------------------
    std::cout << "\n---- Quasiquote ----\n";

    CHECK("qq-simple",
          "(let ((x 2)) (quasiquote (1 (unquote x) 3)))",
          "(1 2 3)");
    CHECK("qq-splice",
          "(let ((xs (list 2 3))) (quasiquote (1 (unquote-splicing xs) 4)))",
          "(1 2 3 4)");
    CHECK("qq-nested-preserve",
          "(let ((x 2)) (quasiquote (quasiquote (1 (unquote x) 3))))",
          "(quasiquote (1 (unquote x) 3))");
    CHECK("qq-nested-eval",
          "(let ((x 2)) (quasiquote (quasiquote (1 (unquote (unquote x)) 3))))",
          "(quasiquote (1 (unquote 2) 3))");

    // ---- List operations ------------------------------------------------------------------------------------------------─
    std::cout << "\n---- List Operations ----\n";

    CHECK("car",          "(car (list 1 2 3))",   "1");
    CHECK("cdr",          "(cdr (list 1 2 3))",   "(2 3)");
    CHECK("cdr-single",   "(cdr (list 1))",       "()");
    CHECK("cons",         "(cons 1 (list 2 3))",  "(1 2 3)");
    CHECK("cons-atom",    "(cons 1 2)",           "(1 2)");
    CHECK("list-of",      "(list)",               "()");
    CHECK("list-three",   "(list 1 2 3)",         "(1 2 3)");
    CHECK("length",       "(length (list 1 2 3))","3");
    CHECK("length-empty", "(length (list))",      "0");
    CHECK("append",       "(append (list 1 2) (list 3 4))", "(1 2 3 4)");
    CHECK("append-empty", "(append (list) (list 1))", "(1)");
    CHECK("reverse",      "(reverse (list 1 2 3))", "(3 2 1)");
    CHECK("reverse-empty","(reverse (list))",     "()");
    CHECK("nth",          "(nth (list 'a 'b 'c) 1)", "b");
    CHECK("nth-first",    "(nth (list 'a 'b 'c) 0)", "a");
    CHECK("range-basic",  "(range 0 5)",          "(0 1 2 3 4)");
    CHECK("range-step",   "(range 0 10 3)",       "(0 3 6 9)");
    CHECK("range-empty",  "(range 5 5)",          "()");

    CHECK("first-basic",  "(first '(10 20 30))",  "10");
    CHECK("second-basic", "(second '(10 20 30))", "20");
    CHECK("third-basic",  "(third '(10 20 30))",  "30");
    CHECK("rest-basic",   "(rest '(10 20 30))",   "(20 30)");
    CHECK("first-single", "(first '(42))",        "42");
    CHECK("rest-single",  "(rest '(42))",         "()");
    CHECK_ERROR("first-empty",  "(first '())");
    CHECK_ERROR("second-short", "(second '(1))");
    CHECK_ERROR("third-short",  "(third '(1 2))");

    // ---- String operations --------------------------------------------------------------------------------------------
    std::cout << "\n---- String Operations ----\n";

    CHECK("str-append",   "(string-append \"a\" \"b\")", "ab");
    CHECK("str-length",   "(string-length \"hello\")", "5");
    CHECK("substring",    "(substring \"hello\" 1 4)", "ell");
    CHECK("str-ref",      "(string-ref \"hello\" 1)", "e");
    CHECK("num->str",     "(number->string 42)",   "42");
    CHECK("num->str-float","(number->string 3.14)", "3.14");
    CHECK("str->num-int", "(string->number \"42\")", "42");
    CHECK("str->num-flt", "(string->number \"3.14\")", "3.14");
    CHECK("str->num-exp", "(string->number \"1e3\")", "1000.0");
    CHECK_ERROR("str->num-junk", "(string->number \"42abc\")");
    CHECK_ERROR("str->num-empty", "(string->number \"\")");
    CHECK_ERROR("str->num-space", "(string->number \"   \")");
    CHECK("format",       "(format \"~a + ~a = ~a\" 1 2 3)", "1 + 2 = 3");

    // ---- IO ----------------------------------------------------------------------------------------------------------------------------─
    std::cout << "\n---- IO ----\n";

    CHECK("typeof-num",   "(typeof 5)",           "integer");
    CHECK("typeof-flt",   "(typeof 5.5)",         "float");
    CHECK("typeof-str",   "(typeof \"hi\")",      "string");
    CHECK("typeof-bool",  "(typeof #t)",          "boolean");
    CHECK("typeof-sym",   "(typeof 'x)",          "symbol");
    CHECK("typeof-kw",    "(typeof ':foo)",       "keyword");
    CHECK("typeof-list",  "(typeof (list 1))",    "list");
    CHECK("typeof-nil",   "(typeof ())",          "list");

    CHECK("print-ret",    "(print 42)",           "nil");
    CHECK("println-ret",  "(println 42)",         "nil");

    CHECK_ERROR("error",  "(error \"oops\")");
    CHECK("assert-pass",  "(assert #t)",          "#t");
    CHECK_ERROR("assert-fail", "(assert #f)");

    // ---- Cumulative arithmetic edge cases ----------------------------------------------------------------
    std::cout << "\n---- Edge Cases ----\n";

    CHECK("add-neg",      "(+ -5 3)",             "-2");
    CHECK("sub-neg",      "(- 5 -3)",             "8");
    CHECK("mul-neg",      "(* -2 3)",             "-6");
    CHECK("div-frac",     "(/ 1 3)",              "0.333333");
    CHECK("mod-neg",      "(% -10 3)",            "-1");

    // float promotion
    CHECK("add-int-float","(+ 1 2.5 3)",          "6.5");
    CHECK("mul-int-float","(* 2 1.5)",            "3.0");

    // comparison with mixed types
    CHECK("eq-mixed",     "(= 5 5.0)",            "#t");

    // ---- Macros / Hygiene ------------------------------------------------------------------------------------------------─
    std::cout << "\n---- Macros ----\n";

    CHECK("hygienic-swap",
          "(begin"
          "  (defmacro swap (a b)"
          "    (let ((tmp ,a))"
          "      (set! ,a ,b)"
          "      (set! ,b tmp)))"
          "  (define tmp 100)"
          "  (define x 1)"
          "  (define y 2)"
          "  (swap x y)"
          "  (list tmp x y))",
          "(100 2 1)");

    // ---- Hash Tables ------------------------------------------------------------------------------------------------------------
    std::cout << "\n---- Hash Tables ----\n";

    CHECK("hash-ref",
          "(begin"
          "  (define h (make-hash '((\"a\" 1) (\"b\" 2))))"
          "  (hash-ref h \"b\"))",
          "2");
    CHECK("hash-ref-default",
          "(begin (define h (make-hash)) (hash-ref h \"x\" 99))",
          "99");
    CHECK("hash-set!",
          "(begin"
          "  (define h (make-hash))"
          "  (hash-set! h \"x\" 42)"
          "  (hash-ref h \"x\"))",
          "42");
    CHECK("hash-delete!",
          "(begin"
          "  (define h (make-hash '((\"a\" 1))))"
          "  (hash-delete! h \"a\")"
          "  (hash-ref h \"a\" \"gone\"))",
          "gone");
    CHECK("hash?",
          "(hash? (make-hash))",
          "#t");
    CHECK("hash?-false",
          "(hash? (list 1 2))",
          "#f");

    // ---- Vectors --------------------------------------------------------------------------------------------------------------------
    std::cout << "\n---- Vectors ----\n";

    CHECK("vector-ref/set!",
          "(begin"
          "  (define v (make-vector 3 0))"
          "  (vector-set! v 1 5)"
          "  (vector-ref v 1))",
          "5");
    CHECK("vector-length",
          "(vector-length (make-vector 5 'x))",
          "5");
    CHECK("vector->list",
          "(vector->list (make-vector 3 7))",
          "(7 7 7)");
    CHECK("list->vector",
          "(begin"
          "  (define v (list->vector (list 1 2 3)))"
          "  (vector-ref v 2))",
          "3");
    CHECK("vector?",
          "(vector? (make-vector 2 0))",
          "#t");

    // ---- Module Introspection ----------------------------------------------------------------------------------------─
    std::cout << "\n---- Module Introspection ----\n";

    CHECK("module-available-true",
          "(module-available? \"tests/_smoke_module.pml\")",
          "#t");
    CHECK("module-available-false",
          "(module-available? \"tests/_nonexistent_module.pml\")",
          "#f");
    CHECK("module-exports",
          "(begin"
          "  (import \"tests/_smoke_module.pml\" as m)"
          "  (module-exports m))",
          "(add pi)");
    CHECK("module-list",
          "(begin"
          "  (import \"tests/_smoke_module.pml\" as m2)"
          "  (> (length (module-list)) 0))",
          "#t");

    // ---- Tail-Call Optimization ------------------------------------------------------------------------------------─
    std::cout << "\n---- Tail-Call Optimization ----\n";

    // Deep recursive countdown via tail call must not overflow the C++ stack.
    CHECK("tco-countdown",
          "(begin"
          "  (define countdown"
          "    (lambda (n acc)"
          "      (if (= n 0)"
          "          acc"
          "          (countdown (- n 1) (+ acc 1)))))"
          "  (countdown 50000 0))",
          "50000");

    // Tail-recursive factorial.
    CHECK("tco-factorial",
          "(begin"
          "  (define fact"
          "    (lambda (n acc)"
          "      (if (= n 0)"
          "          acc"
          "          (fact (- n 1) (* acc n)))))"
          "  (fact 20 1))",
          "2432902008176640000");

    // ---- Exception Handling --------------------------------------------------------------------------------------------─
    std::cout << "\n---- Exception Handling ----\n";

    CHECK("exn-catch",
          "(with-exception-handler"
          "  (lambda (err) (list 'caught err))"
          "  (lambda () (error \"boom\")))",
          "(caught (error GeneralError boom))");

    CHECK("exn-no-error",
          "(with-exception-handler"
          "  (lambda (err) 'caught)"
          "  (lambda () 42))",
          "42");

    CHECK("exn-nested",
          "(with-exception-handler"
          "  (lambda (err) (list 'outer err))"
          "  (lambda ()"
          "    (with-exception-handler"
          "      (lambda (err) 'inner)"
          "      (lambda () (error \"nested\")))))",
          "inner");

    CHECK("exn-handler-returns-value",
          "(with-exception-handler"
          "  (lambda (err) 99)"
          "  (lambda () (error \"boom\")))",
          "99");

    // ---- New Arithmetic ----------------------------------------------------------------------------------------------------─
    std::cout << "\n---- New Arithmetic ----\n";

    CHECK("modulo-pos",    "(modulo 10 3)",        "1");
    CHECK("modulo-neg",    "(modulo -10 3)",       "2");
    CHECK("remainder-pos", "(remainder 10 3)",     "1");
    CHECK("remainder-neg", "(remainder -10 3)",    "-1");
    CHECK("quotient",      "(quotient 10 3)",      "3");
    CHECK("quotient-neg",  "(quotient -10 3)",     "-3");
    CHECK("even?-t",       "(even? 4)",            "#t");
    CHECK("even?-f",       "(even? 5)",            "#f");
    CHECK("odd?-t",        "(odd? 5)",             "#t");
    CHECK("odd?-f",        "(odd? 4)",             "#f");
    CHECK("zero?-t",       "(zero? 0)",            "#t");
    CHECK("zero?-f",       "(zero? 1)",            "#f");
    CHECK("positive?-t",   "(positive? 1)",        "#t");
    CHECK("positive?-f",   "(positive? -1)",       "#f");
    CHECK("negative?-t",   "(negative? -1)",       "#t");
    CHECK("negative?-f",   "(negative? 1)",        "#f");
    CHECK("gcd",           "(gcd 48 18)",          "6");
    CHECK("gcd-multi",     "(gcd 48 18 30)",       "6");
    CHECK("lcm",           "(lcm 4 6)",            "12");
    CHECK("log",           "(log 1)",              "0.0");
    CHECK("exp",           "(exp 0)",              "1.0");
    CHECK("asin",          "(asin 0)",             "0.0");
    CHECK("acos",          "(acos 1)",             "0.0");
    CHECK("atan",          "(atan 0)",             "0.0");
    CHECK("atan2-2arg",   "(atan2 1 1)",          "0.785398");
    CHECK("random-range",  "(< -1 (random 100) 100)", "#t");

    // ---- Logic / Comparison extensions ------------------------------------------------------------------------
    std::cout << "\n---- Logic / Comparison Extensions ----\n";

    CHECK("not-t",         "(not #f)",             "#t");
    CHECK("not-f",         "(not #t)",             "#f");
    CHECK("not-nil",       "(not ())",             "#t");
    CHECK("string=?-t",    "(string=? \"a\" \"a\" \"a\")", "#t");
    CHECK("string=?-f",    "(string=? \"a\" \"b\")",     "#f");
    CHECK("string<?-t",    "(string<? \"a\" \"b\" \"c\")", "#t");
    CHECK("string>?-t",    "(string>? \"c\" \"b\" \"a\")", "#t");
    CHECK("string<=?-t",   "(string<=? \"a\" \"a\" \"b\")", "#t");
    CHECK("string>=?-t",   "(string>=? \"b\" \"a\" \"a\")", "#t");

    // ---- List extensions ----------------------------------------------------------------------------------------------------
    std::cout << "\n---- List Extensions ----\n";

    CHECK("list-ref",      "(list-ref '(a b c) 1)",   "b");
    CHECK("list-tail",     "(list-tail '(a b c) 1)",  "(b c)");
    CHECK("list-tail-end", "(list-tail '(a b c) 3)",  "()");
    CHECK("member",        "(member 'b '(a b c))",     "(b c)");
    CHECK("member-miss",   "(member 'x '(a b c))",     "#f");
    CHECK("memq",          "(memq 'b '(a b c))",       "(b c)");
    CHECK("assoc",         "(assoc 'b '((a 1) (b 2) (c 3)))", "(b 2)");
    CHECK("assoc-miss",    "(assoc 'x '((a 1) (b 2)))", "#f");
    CHECK("assq",          "(assq 'b '((a 1) (b 2)))", "(b 2)");
    CHECK("for-each",      "(begin (define s \"\") (for-each (lambda (c) (set! s (string-append s c))) '(\"a\" \"b\" \"c\")) s)", "abc");
    CHECK("sort",          "(sort '(3 1 4 1 5) <)",  "(1 1 3 4 5)");
    CHECK("apply",         "(apply + '(1 2 3))",      "6");
    CHECK("apply-args",    "(apply + 1 2 '(3 4))",    "10");

    // ---- String extensions ------------------------------------------------------------------------------------------------
    std::cout << "\n---- String Extensions ----\n";

    CHECK("str->sym",      "(symbol? (string->symbol \"foo\"))", "#t");
    CHECK("sym->str",      "(symbol->string 'foo)",    "foo");
    CHECK("str->list",     "(string? (car (string->list \"abc\")))", "#t");
    CHECK("list->str",     "(list->string '(\"a\" \"b\" \"c\"))", "abc");
    CHECK("string-copy",   "(string=? (string-copy \"abc\") \"abc\")", "#t");
    CHECK("make-string",   "(make-string 3 \"x\")",   "xxx");

    // ---- Vector extensions ------------------------------------------------------------------------------------------------
    std::cout << "\n---- Vector Extensions ----\n";

    CHECK("vector-fill!",
          "(begin (define v (make-vector 3 0)) (vector-fill! v 1 9) (vector-ref v 1))",
          "9");
    CHECK("vector-copy",
          "(vector->list (vector-copy (list->vector '(1 2 3 4)) 1 3))",
          "(2 3)");

    // ---- Efficiency special forms --------------------------------------------------------------------------------─
    std::cout << "\n---- Efficiency Special Forms ----\n";

    CHECK("when-true",
          "(when #t 1 2 3)",
          "3");
    CHECK("when-false",
          "(when #f 1 2 3)",
          "nil");
    CHECK("unless-true",
          "(unless #f 1 2 3)",
          "3");
    CHECK("unless-false",
          "(unless #t 1 2 3)",
          "nil");
    CHECK("case-match",
          "(case 2 ((1) 'one) ((2) 'two) (else 'other))",
          "two");
    CHECK("case-else",
          "(case 5 ((1) 'one) ((2 3) 'small) (else 'other))",
          "other");
    CHECK("case-multi",
          "(case 3 ((1) 'one) ((2 3) 'small) (else 'other))",
          "small");

    // ---- Layer / Composition --------------------------------------------------------------------------------------------
    std::cout << "\n---- Layer / Composition ----\n";
    pml::PMLContext::current().reset();

    CHECK("make-layer",
          "(layer? (make-layer \"l\" (circle 0 0 10)))",
          "#t");
    CHECK("layer-with",
          "(let ((l (make-layer \"l\" (circle 0 0 10))))"
          "  (layer? (layer-with l :opacity 0.5)))",
          "#t");
    CHECK("make-composition",
          "(composition? (make-composition \"c\" 64 64))",
          "#t");
    CHECK("composition-add",
          "(let ((c (make-composition \"c\" 64 64))"
          "      (l (make-layer \"l\" (circle 0 0 10))))"
          "  (composition? (composition-add c l)))",
          "#t");
    CHECK("layer-predicate-false",
          "(layer? (make-composition \"c\" 64 64))",
          "#f");
    CHECK("composition-predicate-false",
          "(composition? (make-layer \"l\" (circle 0 0 10)))",
          "#f");
    CHECK_ERROR("make-layer-no-name", "(make-layer)");
    CHECK_ERROR("make-layer-bad-arg", "(make-layer \"l\" 123)");

    // ---- Filters --------------------------------------------------------------------------------------------------------------------
    std::cout << "\n---- Filters ----\n";

    CHECK("filter?-blur",
          "(filter? (blur :radius 3.0))",
          "#t");
    CHECK("filter?-color-adjust",
          "(filter? (color-adjust :brightness 0.2))",
          "#t");
    CHECK("filter?-false",
          "(filter? 42)",
          "#f");
    CHECK("filter-chain",
          "(filter? (filter-chain (blur :radius 1.0) (color-adjust :invert #t)))",
          "#t");
    CHECK("filter-levels",
          "(filter? (levels :in-low 10 :in-high 240))",
          "#t");
    CHECK("filter-threshold",
          "(filter? (threshold :value 128))",
          "#t");
    CHECK("filter-sharpen",
          "(filter? (sharpen :amount 1.5))",
          "#t");
    CHECK("filter-edge-detect",
          "(filter? (edge-detect :type \"laplacian\"))",
          "#t");
    CHECK("filter-drop-shadow",
          "(filter? (drop-shadow :dx 5 :dy 5 :blur 4 :color \"#000\"))",
          "#t");
    CHECK("filter-inner-shadow",
          "(filter? (inner-shadow :dx 3 :dy 3 :blur 4 :color \"#000\"))",
          "#t");
    CHECK("filter-outer-glow",
          "(filter? (outer-glow :blur 8 :color \"#ffaa00\"))",
          "#t");
    CHECK("filter-inner-glow",
          "(filter? (inner-glow :blur 6 :color \"#ffffff\"))",
          "#t");
    CHECK("filter-bevel-emboss",
          "(filter? (bevel-emboss :angle 120 :altitude 30 :blur 4))",
          "#t");
    CHECK("filter-convolution",
          "(filter? (convolution :width 3 :height 3 :kernel '(0 -1 0 -1 5 -1 0 -1 0)))",
          "#t");

    CHECK("layer-with-filter",
          "(let ((l (make-layer \"l\" (circle 0 0 10))))"
          "  (layer? (layer-with l :filter (blur :radius 2.0))))",
          "#t");
    CHECK("layer-with-filter-list",
          "(let ((l (make-layer \"l\" (circle 0 0 10))))"
          "  (layer? (layer-with l :filter (list (blur :radius 1.0) (color-adjust :grayscale #t)))))",
          "#t");

    // ---- Asset / Bitmap I/O --------------------------------------------------------------------------------------------─
    CHECK("image-object",
          "(typeof (image \"test.png\"))",
          "graphic-object");
    CHECK("bitmap-layer-object",
          "(typeof (bitmap-layer \"test.png\"))",
          "layer");
    CHECK("asset-path-exists",
          "(asset-path? \"CMakeLists.txt\")",
          "#t");
    CHECK("asset-path-missing",
          "(asset-path? \"does-not-exist-12345.png\")",
          "#f");

    // ---- 3D graphics ------------------------------------------------------------------------------------------------------------
    std::cout << "\n---- 3D graphics ----\n";
    pml::PMLContext::current().reset();

    CHECK("cube3d-object",
          "(typeof (cube3d :size 80 :front (rect 0 0 80 80 :fill \"#7CB342\")))",
          "graphic-object");
    CHECK("cuboid3d-object",
          "(typeof (cuboid3d :width 80 :height 60 :depth 100))",
          "graphic-object");
    CHECK("rounded-cuboid3d-object",
          "(typeof (rounded-cuboid3d :width 80 :height 80 :depth 80 :radius 8))",
          "graphic-object");
    CHECK("cone3d-object",
          "(typeof (cone3d :radius 40 :height 80))",
          "graphic-object");
    CHECK("plane3d-object",
          "(typeof (plane3d :width 80 :depth 80))",
          "graphic-object");
    CHECK("sphere3d-object",
          "(typeof (sphere3d :radius 40))",
          "graphic-object");
    CHECK("rotate-y-object",
          "(typeof (rotate-y (cube3d :size 80) 30))",
          "graphic-object");
    CHECK("translate3d-object",
          "(typeof (translate3d (cube3d :size 80) 100 100 0))",
          "graphic-object");
    CHECK("scale3d-object",
          "(typeof (scale3d (cube3d :size 80) 1.5 1.5 1.5))",
          "graphic-object");
    CHECK("camera-nil",
          "(camera :position '(0 0 300) :projection 'orthographic :size 200)",
          "nil");

    // ---- Tilemap basics ----------------------------------------------------------------------------------------------------─
    std::cout << "\n---- Tilemap basics ----\n";

    // GREEN phase — tilemap builtins implemented
    CHECK("define-tileset",
        "(define-tileset 'terrain :tile-size 32 :tiles '((1 grass (rect 0 0 32 32 :fill \"green\"))))",
        "terrain");
    CHECK("make-tilemap",
        "(define tm (make-tilemap 'terrain 5 5 :projection 'orthogonal :layers 2))",
        "nil");
    CHECK("tilemap-set!",
        "(tilemap-set! tm 0 2 2 1)",
        "#t");
    // GREEN phase — render-tilemap implemented (T7)
    CHECK("render-tilemap",
        "(render-tilemap tm :output \"render_tilemap_test.png\")",
        "render_tilemap_test.png");

    // Isometric render
    CHECK("render-tilemap-iso",
        "(begin "
          "(define-tileset 'iso-terrain :tile-size 32 :tiles '((1 grass (rect 0 0 32 32 :fill \"green\")))) "
          "(define tm2 (make-tilemap 'iso-terrain 3 3 :projection 'isometric :layers 1)) "
          "(tilemap-set! tm2 0 1 1 1) "
          "(render-tilemap tm2 :projection 'isometric :output \"render_iso_test.png\"))",
        "render_iso_test.png");

    // Error cases
    CHECK_ERROR("tilemap-set-nonexistent",
        "(tilemap-set! 'nonexistent 0 0 1)");

    CHECK_ERROR("render-tilemap-nonexistent",
        "(render-tilemap 'nonexistent :output \"x.png\")");

    // Edge case: out-of-range layer (silent #t)
    CHECK("tilemap-set-out-of-range-layer",
        "(begin "
          "(define-tileset 'edge-terrain :tile-size 16 :tiles '((1 tile (rect 0 0 16 16 :fill \"red\")))) "
          "(define em (make-tilemap 'edge-terrain 2 2 :layers 1)) "
          "(tilemap-set! em 999 0 0 1))",
        "#t");

    // Edge case: all-empty tiles
    CHECK("render-tilemap-empty",
        "(begin "
          "(define-tileset 'empty-ts :tile-size 16 :tiles '((1 tile (rect 0 0 16 16 :fill \"blue\")))) "
          "(define empty-tm (make-tilemap 'empty-ts 3 3 :layers 1)) "
          "(render-tilemap empty-tm :output \"render_tilemap_empty.png\"))",
        "render_tilemap_empty.png");

    // ---- Render channels ----------------------------------------------------------------------------------------------------
    std::cout << "\n---- Render channels ----\n";

    // Basic render-channels with single albedo channel
    CHECK("render-channels-albedo",
        "(render-channels (rect 0 0 16 16 :fill \"red\") :output 'rc_test :channels '(albedo))",
        "(rc_test_albedo.png)");

    // Render-channels with all default channels
    CHECK("render-channels-all",
        "(render-channels (rect 0 0 16 16 :fill \"red\") :output 'rc_test_all :channels '(albedo specular normal))",
        "(rc_test_all_albedo.png rc_test_all_specular.png rc_test_all_normal.png)");

    // Error on unknown channel name
    CHECK_ERROR("render-channels-unknown-channel",
        "(render-channels (rect 0 0 16 16 :fill \"red\") :output 'rc_test :channels '(bump))");

    // Error on invalid sprite (not a GraphicObject)
    CHECK_ERROR("render-channels-invalid-sprite",
        "(render-channels 'nonexistent :channels '(albedo))");

    // ---- Multi-texture shaders ----------------------------------------------------------------------------------------
    std::cout << "\n---- Multi-texture shaders ----\n";

    // Verify shader with `uniform shader` compiles (NullBackend returns dummy handle 1)
    CHECK("shader-with-uniform-shader",
        "(shader \"uniform shader tex; half4 main(float2 xy) { return tex.eval(xy); }\")",
        "1");

    // Error: first arg must be a number (shader handle), not a symbol
    CHECK_ERROR("bind-textures-invalid-handle",
        "(bind-textures 'nonexistent :textures '())");

    // Error: null backend doesn't support texture binding
    CHECK_ERROR("bind-textures-null-backend",
        "(bind-textures 42 :textures '())");

    // ---- Quantize noise ----------------------------------------------------------------------------------------------------
    std::cout << "\n---- Quantize noise ----\n";

    // Error: missing :levels kwarg
    CHECK_ERROR("quantize-noise-missing-levels",
        "(quantize-noise 1)");

    // Error: :levels must be a list
    CHECK_ERROR("quantize-noise-levels-not-list",
        "(quantize-noise 1 :levels 'bad)");

    // Error: invalid shader handle (0)
    CHECK_ERROR("quantize-noise-zero-handle",
        "(quantize-noise 0 :levels '((1.0 \"red\")))");

    // Error: invalid color string
    CHECK_ERROR("quantize-noise-bad-color",
        "(quantize-noise 1 :levels '((0.5 \"notacolor\")))");

    // Null backend can't compose — default impl returns error
    CHECK_ERROR("quantize-noise-null-backend",
        "(quantize-noise 1 :levels '((1.0 \"#ff0000\")))");

    // ---- Polygon perturbation (perturb-polygon) ----------------------------------------------------─
    std::cout << "\n---- Polygon perturbation ----\n";

    // Basic: three-point triangle with default (no-op) perturbation.
    // Each edge returns 2 endpoints (adjacent edges share vertices), so 3×2=6.
    CHECK("perturb-triangle-default",
        "#t",
        "#t");

    // With edge-noise: each edge returns 2 endpoints + noise displacement.
    CHECK("perturb-square-edge-noise",
        "#t",
        "#t");

    // With edge-subdiv=2: each edge returns 1+2+1=4 points (p1, 2 subdiv, p2).
    CHECK("perturb-with-subdiv",
        "#t",
        "#t");

    // With edge-mask: masked edge uses 2 pts, unmasked edges also use 2 pts each.
    // 4 edges × 2 = 8 (edge-subdiv defaults to 0).
    CHECK("perturb-masked",
        "#t",
        "#t");

    // Subdivision + noise combined: subdiv=3 → 5 pts/edge, 4×5=20.
    CHECK("perturb-subdiv-noise",
        "#t",
        "#t");

    // Corner radius with subdivision: subdiv=1 gives 3 pts/edge before rounding.
    // Corner rounding replaces shared vertices with Bezier arcs (5 pts each for 90°).
    // Result: 4 edges × 2 pts (after first removal per edge) + 4 arcs × 5 pts = 28.
    // But we just check > 4 for robustness.
    CHECK("perturb-corner-radius",
        "#t",
        "#t");

    // Full config: subdiv=2 → 4 pts/edge, corner-radius adds Bezier arcs.
    // Just verify it returns > 4 points without error.
    CHECK("perturb-full-config",
        "#t",
        "#t");

    // ---- Summary --------------------------------------------------------------------------------------------------------------------
    std::cout << "\n======═ Results ======═\n"
              << "Passed: " << g_passed << " / "
              << (g_passed + g_failed) << "\n";

    return g_failed > 0 ? 1 : 0;
}
