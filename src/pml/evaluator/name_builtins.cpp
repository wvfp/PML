// ==========================================================================================================================================================================================================================================═
// PML Name Registry Builtins — Implementation
// ==========================================================================================================================================================================================================================================═

#include "pml/evaluator/name_builtins.h"

#include <memory>
#include <string>
#include <unordered_map>

#include "pml/api/context.h"
#include "pml/core/error.h"
#include "pml/core/kwargs.h"
#include "pml/core/types.h"
#include "pml/evaluator/environment.h"
#include "pml/evaluator/evaluator.h"
#include "pml/graphics/name_registry.h"
#include "pml/graphics/objects.h"
#include "pml/graphics/params.h"

namespace pml {

namespace {

using pml::kwargs::kw_double;
using pml::kwargs::kw_string;

// ==========================================================================================================================================================================================================================================═
// name — attach name metadata to a GraphicObject
// ==========================================================================================================================================================================================================================================═
//   (name <name-string> <graphic-object>) → GraphicObject
//
// Attaches metadata["name"] = <name-string> and registers the object in the
// current PMLContext's NameRegistry.  Returns the modified GraphicObject.

static Result<Value> builtin_name(const std::vector<Value>& args,
                                   Environment& /*env*/) {
    if (args.size() != 2) {
        return std::unexpected(arity_error(SourceLocation{}, 2, static_cast<int>(args.size())));
    }

    auto name_opt = kwargs::value_to_opt_string(args[0]);
    if (!name_opt) {
        return std::unexpected(type_error(
            "name: first argument must be a string or symbol"));
    }

    const auto* go_ptr = args[1].as_graphic_object();
    if (!go_ptr || !*go_ptr) {
        return std::unexpected(type_error(
            "name: second argument must be a graphic object (got " +
            value_to_string(args[1]) + ")"));
    }

    // Create a copy with the name set in metadata.
    GraphicObject result = **go_ptr;
    result.metadata["name"] = Value(*name_opt);

    // Register in the current context's NameRegistry.
    PMLContext::current().name_registry->register_object(*name_opt, result);

    return Value(std::make_shared<GraphicObject>(std::move(result)));
}

// ==========================================================================================================================================================================================================================================═
// find — look up a named object
// ==========================================================================================================================================================================================================================================═
//   (find <name-string>) → GraphicObject | nil

static Result<Value> builtin_find(const std::vector<Value>& args,
                                   Environment& /*env*/) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    auto name_opt = kwargs::value_to_opt_string(args[0]);
    if (!name_opt) {
        return std::unexpected(type_error(
            "find: argument must be a string or symbol"));
    }

    auto found = PMLContext::current().name_registry->find(*name_opt);
    if (!found) {
        return make_nil_value();
    }
    return Value(std::make_shared<GraphicObject>(std::move(*found)));
}

// ==========================================================================================================================================================================================================================================═
// remove-name — delete a named object from the registry
// ==========================================================================================================================================================================================================================================═
//   (remove-name <name-string>) → bool (true if existed)

static Result<Value> builtin_remove(const std::vector<Value>& args,
                                     Environment& /*env*/) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    auto name_opt = kwargs::value_to_opt_string(args[0]);
    if (!name_opt) {
        return std::unexpected(type_error(
            "remove: argument must be a string or symbol"));
    }

    bool existed = PMLContext::current().name_registry->remove(*name_opt);
    return Value(static_cast<double>(existed ? 1.0 : 0.0));
}

// ==========================================================================================================================================================================================================================================═
// set-property — modify a named object's properties
// ==========================================================================================================================================================================================================================================═
//   (set-property <name-string> [:fill <color>] [:stroke <color>]
//                                    [:opacity <n>] [:stroke-width <n>])
//
// Looks up the named object, applies modifications via keyword args,
// replaces it in the registry, and returns the new object.

static Result<Value> builtin_set_property(const std::vector<Value>& args,
                                           Environment& /*env*/) {
    if (args.empty()) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    auto name_opt = kwargs::value_to_opt_string(args[0]);
    if (!name_opt) {
        return std::unexpected(type_error(
            "set-property: first argument must be a string or symbol"));
    }

    auto found = PMLContext::current().name_registry->find(*name_opt);
    if (!found) {
        return std::unexpected(PMLException{
            ErrorCode::GeneralError, std::nullopt,
            "set-property: no object named \"" + *name_opt + "\" found",
            std::nullopt});
    }

    GraphicObject obj = std::move(*found);
    auto kwargs = pml::kwargs::parse_kwargs(args, 1);

    // Apply modifications.
    if (auto fill = kw_string(kwargs, "fill", ""); !fill.empty()) {
        obj = obj.with_fill(fill);
    }
    if (auto stroke = kw_string(kwargs, "stroke", ""); !stroke.empty()) {
        obj = obj.with_stroke(stroke);
    }
    if (auto opacity_val = kwargs.find("opacity"); opacity_val != kwargs.end()) {
        auto v = opacity_val->second;
        if (v.is_number()) {
            double opacity = v.is_int() ? static_cast<double>(v.int_val()) : v.double_val();
            obj.opacity = opacity;
        }
    }
    if (auto sw_val = kwargs.find("stroke-width"); sw_val != kwargs.end()) {
        auto v = sw_val->second;
        if (v.is_number()) {
            double sw = v.is_int() ? static_cast<double>(v.int_val()) : v.double_val();
            obj.stroke_width = sw;
        }
    }

    // Replace in registry and return.
    PMLContext::current().name_registry->replace(*name_opt, obj);
    return Value(std::make_shared<GraphicObject>(std::move(obj)));
}

} // anonymous namespace

// ==========================================================================================================================================================================================================================================═
// Registration
// ==========================================================================================================================================================================================================================================═

void register_name_builtins(std::shared_ptr<Environment> env) {
    if (!env) return;

    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn,
                    bool accepts_kwargs = false) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn), accepts_kwargs);
        env->define(name, Value(proc));
    };

    def("name", builtin_name);
    def("lookup", builtin_find);
    def("remove-name", builtin_remove);
    def("set-property", builtin_set_property, true);
}

} // namespace pml
