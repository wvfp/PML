// ═══════════════════════════════════════════════════════════════════════════════
// ImageFilter / FilterChain — implementation
// ═══════════════════════════════════════════════════════════════════════════════

#include "image_filter.h"
#include "filter_backend.h"

namespace pml {

FilterChain::FilterChain(std::vector<std::shared_ptr<ImageFilter>> filters)
    : m_filters(std::move(filters)) {}

Result<void> FilterChain::apply(FilterBackend& backend, Surface& surface) const {
    for (const auto& f : m_filters) {
        if (!f) continue;
        auto r = f->apply(backend, surface);
        if (!r) return r;
    }
    return {};
}

std::string FilterChain::name() const {
    return "filter-chain";
}

const std::vector<std::shared_ptr<ImageFilter>>& FilterChain::filters() const noexcept {
    return m_filters;
}

} // namespace pml
