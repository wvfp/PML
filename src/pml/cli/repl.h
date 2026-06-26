#pragma once

// ==========================================================================================================================================================================================================================================═
// PML REPL — Interactive Read-Eval-Print Loop
//
// Ported from pml/repl.py.  Provides:
//   - Line-by-line interactive evaluation
//   - Multi-line expression support (paren depth tracking)
//   - Structured error display with location info
//   - Persistent environment across evaluations
// ==========================================================================================================================================================================================================================================═

#include <memory>
#include <string>

namespace pml {

class PMLRuntime;
class Environment;

/// Run the interactive REPL (Read-Eval-Print Loop).
///
/// Uses the provided PMLRuntime for evaluation.  The environment persists
/// across REPL inputs — definitions from one line are visible on subsequent
/// lines.
///
/// The loop terminates on EOF (Ctrl-D), (exit), exit, or quit.
///
/// @param runtime  The PMLRuntime instance (provides env + context + arena).
void run_repl(PMLRuntime& runtime);

/// Run a single line of PML input and print the result(s) to stdout.
///
/// Returns true if the line was evaluated successfully (or had a recoverable
/// error), false if the REPL should terminate (e.g. "(exit)").
///
/// @param line     The raw input line from the user.
/// @param runtime  The PMLRuntime instance.
/// @param env      The global PML environment (from runtime.env()).
/// @return False to terminate REPL, true otherwise.
bool run_repl_line(const std::string& line, PMLRuntime& runtime,
                   std::shared_ptr<Environment> env);

/// Accumulate input for multi-line expressions.
///
/// Returns true if the expression is complete (parens balanced), false if
/// more input is needed.  When true, the complete expression is available
/// via accumulated_source().
class LineAccumulator {
public:
    LineAccumulator();

    /// Add a line of input.  Returns true if the expression is complete.
    bool add_line(const std::string& line);

    /// Get the accumulated source string.
    [[nodiscard]] const std::string& source() const noexcept { return m_source; }

    /// Get the current paren depth.
    [[nodiscard]] int depth() const noexcept { return m_depth; }

    /// Check if we're in a multi-line expression.
    [[nodiscard]] bool is_active() const noexcept { return m_depth > 0 || !m_source.empty(); }

    /// Reset the accumulator.
    void reset();

private:
    std::string m_source;
    int m_depth{0};

    /// Update paren depth by scanning the line for ( and ) characters.
    void update_depth(const std::string& line);
};

}  // namespace pml