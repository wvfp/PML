#pragma once

// ═══════════════════════════════════════════════════════════════════════════════
// ImageFilter — base class for all image filters + FilterChain
// ═══════════════════════════════════════════════════════════════════════════════

#include "pml/core/error.h"
#include "pml/backend/backend.h"

#include <memory>
#include <string>
#include <vector>

namespace pml {

class FilterBackend;

// ── ImageFilter ───────────────────────────────────────────────────────────────

class ImageFilter {
public:
    virtual ~ImageFilter() = default;

    /// Apply this filter to `surface` using the provided backend.
    [[nodiscard]] virtual Result<void> apply(
        FilterBackend& backend, Surface& surface) const = 0;

    /// Human-readable filter name.
    [[nodiscard]] virtual std::string name() const = 0;
};

// ── FilterChain ───────────────────────────────────────────────────────────────

class FilterChain final : public ImageFilter {
public:
    explicit FilterChain(std::vector<std::shared_ptr<ImageFilter>> filters);

    [[nodiscard]] Result<void> apply(
        FilterBackend& backend, Surface& surface) const override;
    [[nodiscard]] std::string name() const override;

    [[nodiscard]] const std::vector<std::shared_ptr<ImageFilter>>& filters() const noexcept;

private:
    std::vector<std::shared_ptr<ImageFilter>> m_filters;
};

} // namespace pml
