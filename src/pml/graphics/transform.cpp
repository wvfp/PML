#include "transform.h"

#ifdef PML_USE_SKIA
#include <skia.h>
#endif

namespace pml {

#ifdef PML_USE_SKIA
SkMatrix AffineTransform::to_skmatrix() const
{
    const SkScalar affine[6] = {
        static_cast<SkScalar>(a),
        static_cast<SkScalar>(b),
        static_cast<SkScalar>(c),
        static_cast<SkScalar>(d),
        static_cast<SkScalar>(e),
        static_cast<SkScalar>(f),
    };
    SkMatrix m;
    m.setAffine(affine);
    return m;
}
#endif

// ── Cairo integration ──────────────────────────────────────────────────────

#ifdef PML_USE_CAIRO
cairo_matrix_t AffineTransform::to_cairo_matrix() const
{
    cairo_matrix_t m;
    cairo_matrix_init(&m, a, b, c, d, e, f);
    return m;
}
#endif

}  // namespace pml
