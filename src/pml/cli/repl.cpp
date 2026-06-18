// ═══════════════════════════════════════════════════════════════════════════════
// PML REPL — Interactive Read-Eval-Print Loop Implementation
//
// Ported from pml/repl.py.  Provides line-by-line interactive evaluation
// with multi-line expression support and structured error display.
// ═══════════════════════════════════════════════════════════════════════════════

#include "repl.h"

#include "pml/core/error.h"
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

bool run_repl_line(const std::string& line, std::shared_ptr<Environment> env)
{
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

    // Set __source_file__ for error reporting
    env->define("__source_file__", Value(std::string("<stdin>")));

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
            auto evalResult = evaluate(expr, env);
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

void run_repl(std::shared_ptr<Environment> env)
{
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

        // Execute the accumulated expression
        const auto source = accumulator.source();
        accumulator.reset();

        if (!run_repl_line(source, env)) {
            break;  // exit command
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════════
// Error formatting helper
// ═══════════════════════════════════════════════════════════════════════════════

static std::string format_error(const PMLException& err)
{
    // Match Python format: "filename:line:col: Type: message" or "Type: message"
    std::string result;
    if (err.location.has_value()) {
        const auto& loc = *err.location;
        if (!loc.filename.empty()) {
            result += loc.filename + ":";
        }
        if (loc.line > 0) {
            result += std::to_string(loc.line) + ":";
            if (loc.column > 0) {
                result += std::to_string(loc.column) + ": ";
            } else {
                result += " ";
            }
        }
    }
    result += to_string(err.code);
    result += ": ";
    result += err.message;

    if (err.repair_hint.has_value() && !err.repair_hint->empty()) {
        result += "\n  Hint: ";
        result += *err.repair_hint;
    }

    return result;
}

}  // namespace pml
