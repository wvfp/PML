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

}  // namespace pml
