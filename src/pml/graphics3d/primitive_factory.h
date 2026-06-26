#pragma once

// ==========================================================================================================================================================================================================================================═
// 3D primitive factories
// ==========================================================================================================================================================================================================================================═

#include "mesh3d.h"
#include "pml/graphics/objects.h"

#include <memory>

namespace pml {

[[nodiscard]] std::shared_ptr<Mesh3D> make_cube(
    double size,
    const GraphicObject& front,
    const GraphicObject& back,
    const GraphicObject& left,
    const GraphicObject& right,
    const GraphicObject& top,
    const GraphicObject& bottom);

[[nodiscard]] std::shared_ptr<Mesh3D> make_cuboid(
    double width,
    double height,
    double depth,
    const GraphicObject& front,
    const GraphicObject& back,
    const GraphicObject& left,
    const GraphicObject& right,
    const GraphicObject& top,
    const GraphicObject& bottom);

[[nodiscard]] std::shared_ptr<Mesh3D> make_rounded_cuboid(
    double width,
    double height,
    double depth,
    double radius,
    int segments,
    const GraphicObject& front,
    const GraphicObject& back,
    const GraphicObject& left,
    const GraphicObject& right,
    const GraphicObject& top,
    const GraphicObject& bottom);

[[nodiscard]] std::shared_ptr<Mesh3D> make_cone(
    double radius,
    double height,
    int segments,
    const GraphicObject& side,
    const GraphicObject& base);

[[nodiscard]] std::shared_ptr<Mesh3D> make_plane(
    double width,
    double depth,
    const GraphicObject& material);

[[nodiscard]] std::shared_ptr<Mesh3D> make_sphere(
    double radius,
    int segments,
    int rings,
    const GraphicObject& material);

} // namespace pml
