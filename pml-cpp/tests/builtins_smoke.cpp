// ═══════════════════════════════════════════════════════════════════════════════
// PML Built-in Procedures — Smoke Test
// ═══════════════════════════════════════════════════════════════════════════════
//
// Verifies that built-in procedures registered via register_builtins() work
// correctly through the full lexer → parser → evaluator pipeline.
// No Google Test dependency — standalone compile-and-run.
// ═══════════════════════════════════════════════════════════════════════════════

#include "builtins.h"
#include "evaluator.h"
#include "lexer.h"
#include "parser.h"
#include "environment.h"
#include "types.h"

#include <cassert>
#include <iostream>
#include <memory>
#include <string>
#include <format>

// ── Test helpers ─────────────────────────────────────────────────────────────

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
    return pml::evaluate(*expr, env);
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

    std::cout << "═══ PML Builtins Smoke Test ═══\n\n";

    // ── Arithmetic ───────────────────────────────────────────────────────
    std::cout << "── Arithmetic ──\n";

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

    // ── Comparison ──────────────────────────────────────────────────────
    std::cout << "\n── Comparison ──\n";

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

    // ── Type predicates ─────────────────────────────────────────────────
    std::cout << "\n── Type Predicates ──\n";

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

    // ── List operations ─────────────────────────────────────────────────
    std::cout << "\n── List Operations ──\n";

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

    // ── String operations ──────────────────────────────────────────────
    std::cout << "\n── String Operations ──\n";

    CHECK("str-append",   "(string-append \"a\" \"b\")", "ab");
    CHECK("str-length",   "(string-length \"hello\")", "5");
    CHECK("substring",    "(substring \"hello\" 1 4)", "ell");
    CHECK("str-ref",      "(string-ref \"hello\" 1)", "e");
    CHECK("num->str",     "(number->string 42)",   "42");
    CHECK("num->str-float","(number->string 3.14)", "3.14");
    CHECK("str->num-int", "(string->number \"42\")", "42");
    CHECK("str->num-flt", "(string->number \"3.14\")", "3.14");
    CHECK("format",       "(format \"~a + ~a = ~a\" 1 2 3)", "1 + 2 = 3");

    // ── IO ───────────────────────────────────────────────────────────────
    std::cout << "\n── IO ──\n";

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

    // ── Cumulative arithmetic edge cases ────────────────────────────────
    std::cout << "\n── Edge Cases ──\n";

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

    // ── Summary ──────────────────────────────────────────────────────────
    std::cout << "\n═══ Results ═══\n"
              << "Passed: " << g_passed << " / "
              << (g_passed + g_failed) << "\n";

    return g_failed > 0 ? 1 : 0;
}
