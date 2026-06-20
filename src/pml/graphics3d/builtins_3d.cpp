#include "builtins_3d.h"

#include "camera3d.h"
#include "mesh3d.h"
#include "primitive_factory.h"
#include "transform3d.h"
#include "pml/core/error.h"
#include "pml/core/kwargs.h"
#include "pml/evaluator/evaluator.h"
#include "pml/graphics/objects.h"

#include <memory>

namespace pml {

using namespace pml::kwargs;

namespace {

double to_double(const Value& v) {
    if (v.is_int()) return static_cast<double>(v.int_val());
    if (v.is_double()) return v.double_val();
    return 0.0;
}

const GraphicObject& get_material(
    const std::unordered_map<std::string, Value>& kwargs,
    const std::string& key,
    const GraphicObject& default_material) {
    auto it = kwargs.find(key);
    if (it == kwargs.end()) return default_material;
    const auto* go = it->second.as_graphic_object();
    if (!go) return default_material;
    return **go;
}

GraphicObject make_default_material(double size, const std::string& color) {
    return GraphicObject(
        "rect",
        Params{{"x", Value(0.0)}, {"y", Value(0.0)},
               {"w", Value(size)}, {"h", Value(size)}},
        color,
        std::nullopt,
        0.0);
}

Result<GraphicObject> extract_mesh3d_object(const Value& v, const std::string& fn_name) {
    const auto* go = v.as_graphic_object();
    if (!go || (*go)->shape_type != "mesh3d") {
        return std::unexpected(type_error(
            fn_name + ": expected mesh3d graphic object"));
    }
    return **go;
}

Result<std::shared_ptr<Mesh3D>> extract_mesh(const GraphicObject& obj, const std::string& fn_name) {
    const Value* mesh_val = obj.params.find(ParamKey::mesh);
    if (!mesh_val) {
        return std::unexpected(type_error(fn_name + ": mesh3d missing mesh param"));
    }
    const auto* mesh = mesh_val->as_mesh3d();
    if (!mesh) {
        return std::unexpected(type_error(fn_name + ": mesh param is not a Mesh3D"));
    }
    return *mesh;
}

Result<std::shared_ptr<Transform3D>> extract_transform(const GraphicObject& obj, const std::string& fn_name) {
    const Value* transform_val = obj.params.find(ParamKey::transform);
    if (!transform_val) {
        return std::unexpected(type_error(fn_name + ": mesh3d missing transform param"));
    }
    const auto* transform = transform_val->as_transform3d();
    if (!transform) {
        return std::unexpected(type_error(fn_name + ": transform param is not a Transform3D"));
    }
    return *transform;
}

Result<Value> make_mesh3d_value(std::shared_ptr<Mesh3D> mesh, std::shared_ptr<Transform3D> transform) {
    Params params;
    params.set("mesh", Value(mesh));
    params.set("transform", Value(transform));
    GraphicObject obj("mesh3d", params, std::nullopt, std::nullopt, 0.0);
    return Value(std::make_shared<GraphicObject>(std::move(obj)));
}

Result<Value> builtin_cube3d(const std::vector<Value>& args, Environment&) {
    auto kwargs = parse_kwargs(args, 0);
    double size = kw_double(kwargs, "size", 80.0);

    auto default_mat = make_default_material(size, "#808080");
    const GraphicObject& front  = get_material(kwargs, "front",  default_mat);
    const GraphicObject& back   = get_material(kwargs, "back",   default_mat);
    const GraphicObject& left   = get_material(kwargs, "left",   default_mat);
    const GraphicObject& right  = get_material(kwargs, "right",  default_mat);
    const GraphicObject& top    = get_material(kwargs, "top",    default_mat);
    const GraphicObject& bottom = get_material(kwargs, "bottom", default_mat);

    auto mesh = make_cube(size, front, back, left, right, top, bottom);
    return make_mesh3d_value(mesh, std::make_shared<Transform3D>(Transform3D::identity()));
}

Result<Value> builtin_rounded_cuboid3d(const std::vector<Value>& args, Environment&) {
    auto kwargs = parse_kwargs(args, 0);
    double width   = kw_double(kwargs, "width",  80.0);
    double height  = kw_double(kwargs, "height", 80.0);
    double depth   = kw_double(kwargs, "depth",  80.0);
    double radius  = kw_double(kwargs, "radius", 8.0);
    int segments   = kw_int(kwargs, "segments", 4);

    auto default_mat = make_default_material(std::max({width, height, depth}), "#808080");
    const GraphicObject& front  = get_material(kwargs, "front",  default_mat);
    const GraphicObject& back   = get_material(kwargs, "back",   default_mat);
    const GraphicObject& left   = get_material(kwargs, "left",   default_mat);
    const GraphicObject& right  = get_material(kwargs, "right",  default_mat);
    const GraphicObject& top    = get_material(kwargs, "top",    default_mat);
    const GraphicObject& bottom = get_material(kwargs, "bottom", default_mat);

    auto mesh = make_rounded_cuboid(width, height, depth, radius, segments,
                                    front, back, left, right, top, bottom);
    return make_mesh3d_value(mesh, std::make_shared<Transform3D>(Transform3D::identity()));
}

Result<Value> builtin_cone3d(const std::vector<Value>& args, Environment&) {
    auto kwargs = parse_kwargs(args, 0);
    double radius  = kw_double(kwargs, "radius",  40.0);
    double height  = kw_double(kwargs, "height",  80.0);
    int segments   = kw_int(kwargs, "segments", 32);

    auto default_mat = make_default_material(std::max(radius * 2.0, height), "#808080");
    const GraphicObject& side = get_material(kwargs, "side", default_mat);
    const GraphicObject& base = get_material(kwargs, "base", default_mat);

    auto mesh = make_cone(radius, height, segments, side, base);
    return make_mesh3d_value(mesh, std::make_shared<Transform3D>(Transform3D::identity()));
}

Result<Value> builtin_plane3d(const std::vector<Value>& args, Environment&) {
    auto kwargs = parse_kwargs(args, 0);
    double width  = kw_double(kwargs, "width",  80.0);
    double depth  = kw_double(kwargs, "depth",  80.0);

    auto default_mat = make_default_material(std::max(width, depth), "#808080");
    const GraphicObject& material = get_material(kwargs, "material", default_mat);

    auto mesh = make_plane(width, depth, material);
    return make_mesh3d_value(mesh, std::make_shared<Transform3D>(Transform3D::identity()));
}

Result<Value> builtin_sphere3d(const std::vector<Value>& args, Environment&) {
    auto kwargs = parse_kwargs(args, 0);
    double radius  = kw_double(kwargs, "radius",  40.0);
    int segments   = kw_int(kwargs, "segments", 32);
    int rings      = kw_int(kwargs, "rings", 16);

    auto default_mat = make_default_material(radius * 2.0, "#808080");
    const GraphicObject& material = get_material(kwargs, "material", default_mat);

    auto mesh = make_sphere(radius, segments, rings, material);
    return make_mesh3d_value(mesh, std::make_shared<Transform3D>(Transform3D::identity()));
}

Result<Value> builtin_cuboid3d(const std::vector<Value>& args, Environment&) {
    auto kwargs = parse_kwargs(args, 0);
    double width  = kw_double(kwargs, "width",  80.0);
    double height = kw_double(kwargs, "height", 80.0);
    double depth  = kw_double(kwargs, "depth",  80.0);

    auto default_mat = make_default_material(std::max({width, height, depth}), "#808080");
    const GraphicObject& front  = get_material(kwargs, "front",  default_mat);
    const GraphicObject& back   = get_material(kwargs, "back",   default_mat);
    const GraphicObject& left   = get_material(kwargs, "left",   default_mat);
    const GraphicObject& right  = get_material(kwargs, "right",  default_mat);
    const GraphicObject& top    = get_material(kwargs, "top",    default_mat);
    const GraphicObject& bottom = get_material(kwargs, "bottom", default_mat);

    auto mesh = make_cuboid(width, height, depth, front, back, left, right, top, bottom);
    return make_mesh3d_value(mesh, std::make_shared<Transform3D>(Transform3D::identity()));
}

Result<Value> builtin_rotate_x(const std::vector<Value>& args, Environment&) {
    if (args.size() < 2) return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
    auto obj_res = extract_mesh3d_object(args[0], "rotate-x");
    if (!obj_res) return std::unexpected(obj_res.error());
    double angle = to_double(args[1]);

    auto mesh_res = extract_mesh(*obj_res, "rotate-x");
    auto transform_res = extract_transform(*obj_res, "rotate-x");
    if (!mesh_res) return std::unexpected(mesh_res.error());
    if (!transform_res) return std::unexpected(transform_res.error());

    auto new_transform = std::make_shared<Transform3D>((*transform_res)->rotated_x(angle));
    return make_mesh3d_value(*mesh_res, new_transform);
}

Result<Value> builtin_rotate_y(const std::vector<Value>& args, Environment&) {
    if (args.size() < 2) return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
    auto obj_res = extract_mesh3d_object(args[0], "rotate-y");
    if (!obj_res) return std::unexpected(obj_res.error());
    double angle = to_double(args[1]);

    auto mesh_res = extract_mesh(*obj_res, "rotate-y");
    auto transform_res = extract_transform(*obj_res, "rotate-y");
    if (!mesh_res) return std::unexpected(mesh_res.error());
    if (!transform_res) return std::unexpected(transform_res.error());

    auto new_transform = std::make_shared<Transform3D>((*transform_res)->rotated_y(angle));
    return make_mesh3d_value(*mesh_res, new_transform);
}

Result<Value> builtin_rotate_z(const std::vector<Value>& args, Environment&) {
    if (args.size() < 2) return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
    auto obj_res = extract_mesh3d_object(args[0], "rotate-z");
    if (!obj_res) return std::unexpected(obj_res.error());
    double angle = to_double(args[1]);

    auto mesh_res = extract_mesh(*obj_res, "rotate-z");
    auto transform_res = extract_transform(*obj_res, "rotate-z");
    if (!mesh_res) return std::unexpected(mesh_res.error());
    if (!transform_res) return std::unexpected(transform_res.error());

    auto new_transform = std::make_shared<Transform3D>((*transform_res)->rotated_z(angle));
    return make_mesh3d_value(*mesh_res, new_transform);
}

Result<Value> builtin_translate3d(const std::vector<Value>& args, Environment&) {
    if (args.size() < 4) return std::unexpected(arity_error(SourceLocation{}, 4, static_cast<int>(args.size())));
    auto obj_res = extract_mesh3d_object(args[0], "translate3d");
    if (!obj_res) return std::unexpected(obj_res.error());
    double x = to_double(args[1]);
    double y = to_double(args[2]);
    double z = to_double(args[3]);

    auto mesh_res = extract_mesh(*obj_res, "translate3d");
    auto transform_res = extract_transform(*obj_res, "translate3d");
    if (!mesh_res) return std::unexpected(mesh_res.error());
    if (!transform_res) return std::unexpected(transform_res.error());

    auto new_transform = std::make_shared<Transform3D>((*transform_res)->translated(x, y, z));
    return make_mesh3d_value(*mesh_res, new_transform);
}

Result<Value> builtin_scale3d(const std::vector<Value>& args, Environment&) {
    if (args.size() < 4) return std::unexpected(arity_error(SourceLocation{}, 4, static_cast<int>(args.size())));
    auto obj_res = extract_mesh3d_object(args[0], "scale3d");
    if (!obj_res) return std::unexpected(obj_res.error());
    double sx = to_double(args[1]);
    double sy = to_double(args[2]);
    double sz = to_double(args[3]);

    auto mesh_res = extract_mesh(*obj_res, "scale3d");
    auto transform_res = extract_transform(*obj_res, "scale3d");
    if (!mesh_res) return std::unexpected(mesh_res.error());
    if (!transform_res) return std::unexpected(transform_res.error());

    auto new_transform = std::make_shared<Transform3D>((*transform_res)->scaled(sx, sy, sz));
    return make_mesh3d_value(*mesh_res, new_transform);
}

Result<Value> builtin_camera(const std::vector<Value>& args, Environment&) {
    auto kwargs = parse_kwargs(args, 0);
    Camera3D& cam = current_camera();

    auto pos = kwargs.find("position");
    if (pos != kwargs.end()) {
        if (auto* lst = pos->second.as_list()) {
            auto& elems = (*lst)->elements;
            if (elems.size() >= 3) {
                cam.position = Vec3(
                    to_double(elems[0]),
                    to_double(elems[1]),
                    to_double(elems[2]));
            }
        }
    }

    auto target = kwargs.find("target");
    if (target != kwargs.end()) {
        if (auto* lst = target->second.as_list()) {
            auto& elems = (*lst)->elements;
            if (elems.size() >= 3) {
                cam.target = Vec3(
                    to_double(elems[0]),
                    to_double(elems[1]),
                    to_double(elems[2]));
            }
        }
    }

    auto up = kwargs.find("up");
    if (up != kwargs.end()) {
        if (auto* lst = up->second.as_list()) {
            auto& elems = (*lst)->elements;
            if (elems.size() >= 3) {
                cam.up = Vec3(
                    to_double(elems[0]),
                    elems[1].is_number() ? to_double(elems[1]) : 1.0,
                    to_double(elems[2]));
            }
        }
    }

    cam.fov         = kw_double(kwargs, "fov", cam.fov);
    if (kwargs.find("size") != kwargs.end()) {
        cam.ortho_size = kw_double(kwargs, "size", cam.ortho_size);
        cam.user_ortho_size = true;
    }
    cam.near_plane  = kw_double(kwargs, "near", cam.near_plane);
    cam.far_plane   = kw_double(kwargs, "far", cam.far_plane);

    auto proj = kwargs.find("projection");
    if (proj != kwargs.end()) {
        auto sym = proj->second.as_symbol();
        if (sym && sym->name == "orthographic") {
            cam.projection = Camera3D::Projection::Orthographic;
        } else if (sym && sym->name == "perspective") {
            cam.projection = Camera3D::Projection::Perspective;
        }
    }

    auto cull = kwargs.find("backface-culling");
    if (cull != kwargs.end()) {
        cam.backface_culling = is_truthy(cull->second);
    }

    return Value(nullptr); // nil
}

} // anonymous namespace

void register_3d_builtins(std::shared_ptr<Environment> env) {
    env->define("cube3d", Value(std::make_shared<BuiltinProcedure>("cube3d", builtin_cube3d, true)));
    env->define("cuboid3d", Value(std::make_shared<BuiltinProcedure>("cuboid3d", builtin_cuboid3d, true)));
    env->define("rounded-cuboid3d", Value(std::make_shared<BuiltinProcedure>("rounded-cuboid3d", builtin_rounded_cuboid3d, true)));
    env->define("cone3d", Value(std::make_shared<BuiltinProcedure>("cone3d", builtin_cone3d, true)));
    env->define("plane3d", Value(std::make_shared<BuiltinProcedure>("plane3d", builtin_plane3d, true)));
    env->define("sphere3d", Value(std::make_shared<BuiltinProcedure>("sphere3d", builtin_sphere3d, true)));
    env->define("rotate-x", Value(std::make_shared<BuiltinProcedure>("rotate-x", builtin_rotate_x, false)));
    env->define("rotate-y", Value(std::make_shared<BuiltinProcedure>("rotate-y", builtin_rotate_y, false)));
    env->define("rotate-z", Value(std::make_shared<BuiltinProcedure>("rotate-z", builtin_rotate_z, false)));
    env->define("translate3d", Value(std::make_shared<BuiltinProcedure>("translate3d", builtin_translate3d, false)));
    env->define("scale3d", Value(std::make_shared<BuiltinProcedure>("scale3d", builtin_scale3d, false)));
    env->define("camera", Value(std::make_shared<BuiltinProcedure>("camera", builtin_camera, true)));
}

} // namespace pml
