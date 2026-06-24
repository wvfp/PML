// ═══════════════════════════════════════════════════════════════════════════════
// PML REPL — Interactive Read-Eval-Print Loop Implementation
//
// Ported from pml/repl.py.  Provides line-by-line interactive evaluation
// with multi-line expression support and structured error display.
//
// Unlike file/JSON execution modes, the REPL evaluates each line through
// PMLRuntime::execute() to ensure consistent context (arena, call stack,
// canvas/timeline state) with the other modes.
// ═══════════════════════════════════════════════════════════════════════════════

#include "repl.h"

#include "pml/api/api.h"
#include "pml/api/context.h"
#include "pml/core/arena.h"
#include "pml/core/call_stack.h"
#include "pml/core/error.h"
#include "pml/core/source_manager.h"
#include "pml/core/types.h"
#include "pml/evaluator/evaluator.h"
#include "pml/frontend/expander.h"
#include "pml/frontend/lexer.h"
#include "pml/frontend/parser.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <memory>
#include <string>

namespace pml {

// Source cache for REPL error snippets.
static SourceManager g_repl_source_manager;

// Forward declaration (defined below)
static std::string format_error(const PMLException& err);

// ═══════════════════════════════════════════════════════════════════════════════
// LineAccumulator — multi-line expression support
// ═══════════════════════════════════════════════════════════════════════════════

LineAccumulator::LineAccumulator() = default;

void LineAccumulator::update_depth(const std::string& line)
{
    bool in_string = false;
    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            in_string = !in_string;
        } else if (!in_string) {
            if (c == '(') {
                ++m_depth;
            } else if (c == ')') {
                if (m_depth > 0) --m_depth;
            }
        }
    }
}

bool LineAccumulator::add_line(const std::string& line)
{
    if (!m_source.empty()) {
        m_source += '\n';
    }
    m_source += line;
    update_depth(line);
    // Expression is complete when parens are balanced and we have content
    return m_depth == 0 && !m_source.empty();
}

void LineAccumulator::reset()
{
    m_source.clear();
    m_depth = 0;
}

// ═══════════════════════════════════════════════════════════════════════════════
// REPL line evaluation
// ═══════════════════════════════════════════════════════════════════════════════

bool run_repl_line(const std::string& line, PMLRuntime& runtime,
                   std::shared_ptr<Environment> env)
{
    // Cache the source so error formatters can display it.
    g_repl_source_manager.load_source("<stdin>", line);

    // Trim whitespace
    std::string trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\n\r"));
    trimmed.erase(trimmed.find_last_not_of(" \t\n\r") + 1);

    if (trimmed.empty()) {
        return true;  // skip blank lines
    }

    // Check for exit commands
    if (trimmed == "(exit)" || trimmed == "exit" || trimmed == "quit") {
        return false;
    }

    // ── Context setup (matching PMLRuntime::execute()) ────────────────────
    // Reset the runtime call stack so errors from previous lines do not leak.
    CallStack::instance().clear();

    // Activate this runtime's context so canvas/timeline/styles accessors
    // see the correct per-runtime state.
    PMLContextScope ctx_scope(runtime.context());

    // Activate the per-runtime arena for short-lived AST/eval temporaries.
    // The arena resets when this scope ends (after printing results).
    ArenaScope arena_scope(&runtime.arena());

    // Set __source_file__ for error reporting in imported modules.
    env->define("__source_file__", Value(std::string("<stdin>")));

    // ── Evaluate using low-level pipeline for per-expression control ──────
    // Unlike PMLRuntime::execute(), this loop prints every expression's value
    // and continues after errors — appropriate for interactive use.

    try {
        // Tokenize
        auto tokenResult = Lexer(trimmed, "<stdin>").tokenize();
        if (!tokenResult.has_value()) {
            const auto& err = tokenResult.error();
            std::cerr << format_error(err) << std::endl;
            return true;
        }
        auto& tokens = *tokenResult;

        // Parse
        auto parseResult = Parser(std::move(tokens), "<stdin>").parse();
        if (!parseResult.has_value()) {
            const auto& err = parseResult.error();
            std::cerr << format_error(err) << std::endl;
            return true;
        }
        auto ast = std::move(*parseResult);

        // Expand macros
        Expander expander(env);
        auto expandResult = expander.expand_all(ast);
        if (!expandResult.has_value()) {
            const auto& err = expandResult.error();
            std::cerr << format_error(err) << std::endl;
            return true;
        }
        auto expanded = std::move(*expandResult);

        // Evaluate each expression, printing results
        for (const auto& expr : expanded) {
            auto evalResult = eval_to_value(expr, env);
            if (!evalResult.has_value()) {
                const auto& err = evalResult.error();
                std::cerr << format_error(err) << std::endl;
                // Continue to next expression despite error (like Python REPL)
                continue;
            }
            const auto& value = *evalResult;
            if (!is_nil(value)) {
                std::cout << value_to_string(value) << std::endl;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Internal error: " << e.what() << std::endl;
    }

    return true;
}

// ═══════════════════════════════════════════════════════════════════════════════
// Main REPL loop
// ═══════════════════════════════════════════════════════════════════════════════

void run_repl(PMLRuntime& runtime)
{
    auto env = runtime.env();
    std::cout << "PML 0.1.0 — Type (exit) or Ctrl-D to quit." << std::endl;

    LineAccumulator accumulator;
    std::string line;

    while (true) {
        // Show prompt based on accumulator state
        if (accumulator.is_active()) {
            std::cout << "     ";  // continuation prompt
        } else {
            std::cout << "pml> ";
        }
        std::cout.flush();

        // Read a line
        if (!std::getline(std::cin, line)) {
            // EOF or error
            if (std::cin.eof()) {
                std::cout << std::endl;
            }
            break;
        }

        // Add to accumulator
        bool complete = accumulator.add_line(line);

        if (!complete) {
            continue;  // need more lines
        }

        // Execute the accumulated expression via PMLRuntime
        const auto source = accumulator.source();
        accumulator.reset();

        if (!run_repl_line(source, runtime, env)) {
            break;  // exit command
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Error formatting helper
// ═══════════════════════════════════════════════════════════════════════════════

static std::string format_error(const PMLException& err)
{
    // Include a source snippet when the REPL input is cached.
    return format_error_with_source(err, g_repl_source_manager);
}

}  // namespace pml