#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Built-in Procedures Registration
// ───────────────────────────────────────────────────────────────────────────────
// Registers all built-in PML procedures into an environment:
//   Arithmetic, comparison, IO, list ops, string ops, type predicates.
//
// Each builtin is registered as a BuiltinProcedure with the standard
// signature: Result<Value>(const std::vector<Value>&, Environment&).
// ═══════════════════════════════════════════════════════════════════════════════

#include "environment.h"

#include <memory>

namespace pml {

/// Register all built-in procedures into the given environment.
/// This includes arithmetic (+, -, *, /, %), comparison (=, <, >, eq?, equal?),
/// IO (print, println, error, typeof), list operations (car, cdr, cons, ...),
/// string operations (string-append, format, ...), and type predicates.
void register_builtins(std::shared_ptr<Environment> env);

/// Convenience overload: takes a reference (wraps in shared_ptr).
void register_builtins(Environment& env);

}  // namespace pml
