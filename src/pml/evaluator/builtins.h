#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Built-in Procedures Registration
// ------------------------------------------------------------------------------------------------------------------------------------------------------------─
// Each category of builtins is registered by a dedicated function, grouped
// in separate compilation units for faster builds and clearer responsibilities.
//
//   Arithmetic  — +, -, *, /, %, comparison, trigonometry, math extensions
//   IO          — print, println, error, typeof, assert
//   List        — car, cdr, cons, map, filter, reduce, ...
//   String      — string-append, string-length, format, ...
//   Predicates  — number?, string?, list?, module-available?, ...
//   Containers  — hash tables, vectors, vector-fill!, vector-copy
// ==========================================================================================================================================================================================================================================═

#include "environment.h"

#include <memory>

namespace pml {

/// Register arithmetic, comparison, and math-extension builtins.
void register_arithmetic_builtins(std::shared_ptr<Environment> env);

/// Register IO / debugging builtins (print, println, error, typeof, assert).
void register_io_builtins(std::shared_ptr<Environment> env);

/// Register list-operation and higher-order list builtins.
void register_list_builtins(std::shared_ptr<Environment> env);

/// Register string-operation builtins.
void register_string_builtins(std::shared_ptr<Environment> env);

/// Register type predicate and module introspection builtins.
void register_predicates_builtins(std::shared_ptr<Environment> env);

/// Register container (hash table + vector) builtins.
void register_container_builtins(std::shared_ptr<Environment> env);

/// Register all built-in procedures by calling all category registrations.
void register_builtins(std::shared_ptr<Environment> env);

/// Convenience overload: takes a reference (wraps in shared_ptr).
void register_builtins(Environment& env);

}  // namespace pml
