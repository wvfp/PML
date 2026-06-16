#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Expander — Macro Expansion Pass
// ═══════════════════════════════════════════════════════════════════════════════
//
// Pre-evaluation pass that recursively expands macro calls in the AST.
// Non-hygienic: macro authors must use (gensym) for unique variable names.
//
// Matches the Python PML expander (pml/expander.py) exactly.
//
// Expansion algorithm:
//   1. If expr is not a list or is empty → return as-is
//   2. If head is a Symbol resolving to a Macro → expand and re-expand result
//   3. Otherwise → recursively expand all sub-expressions
// ═══════════════════════════════════════════════════════════════════════════════

#include "error.h"
#include "types.h"

#include <memory>
#include <vector>

namespace pml {

// Forward declaration — defined in pml/evaluator/environment.h
class Environment;

/// Macro expander that runs before evaluation.
///
/// Recursively expands macro calls in the AST. Non-hygienic:
/// macro authors must use (gensym) for unique variable names.
class Expander {
public:
    /// @param env       Environment for looking up macro definitions.
    /// @param max_depth Maximum macro expansion nesting depth (default 256).
    explicit Expander(std::shared_ptr<Environment> env, int max_depth = 256);

    /// Expand a single expression, recursively expanding macro calls.
    /// Returns the fully expanded expression.
    [[nodiscard]] Result<Expr> expand(const Expr& expr, int depth = 0) const;

    /// Expand macros in a list of top-level expressions.
    [[nodiscard]] Result<std::vector<Expr>> expand_all(
        const std::vector<Expr>& ast) const;

private:
    std::shared_ptr<Environment> env;
    int max_depth;
};

}  // namespace pml
