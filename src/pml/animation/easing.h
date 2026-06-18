#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// PML Animation Easing Functions
// ───────────────────────────────────────────────────────────────────────────────
// Each easing function maps t ∈ [0, 1] → [0, 1], where:
//   - t=0 → start
//   - t=1 → end
//
// Ported from pml/animation/easing.py. IEEE 754 double precision.
// ═══════════════════════════════════════════════════════════════════════════════

#include <functional>
#include <string>
#include <vector>

namespace pml {

using EasingFn = std::function<double(double)>;

// ── Built-in easing functions ────────────────────────────────────────────────

double easing_linear(double t);
double easing_quad_in(double t);
double easing_quad_out(double t);
double easing_quad_in_out(double t);
double easing_cubic_in(double t);
double easing_cubic_out(double t);
double easing_cubic_in_out(double t);
double easing_sin_in(double t);
double easing_sin_out(double t);
double easing_sin_in_out(double t);
double easing_bounce(double t);
double easing_elastic(double t);

// ── Lookup ───────────────────────────────────────────────────────────────────

/// Return an easing function by name (case-sensitive, hyphenated names
/// matching Python: "linear", "quad-in", "quad-out", etc.).
/// Falls back to easing_linear if the name is not recognized.
EasingFn get_easing(const std::string& name);

/// Return all available easing function names, sorted alphabetically.
std::vector<std::string> list_easings();

} // namespace pml
