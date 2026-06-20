#include "backend_builtins.h"

#include "environment.h"
#include "types.h"
#include "error.h"
#include "../backend/registry.h"
#include "../backend/capabilities.h"

namespace pml {

namespace {

Result<Value> builtin_list_backends(const std::vector<Value>&, Environment&) {
    auto& registry = BackendRegistry::instance();
    auto backends = registry.available();

    std::vector<Value> result;
    result.reserve(backends.size());

    for (const auto& info : backends) {
        std::vector<Value> entry;
        entry.push_back(Value(info.name));
        entry.push_back(Value(info.description));
        result.push_back(make_list_value(std::move(entry)));
    }

    return make_list_value(std::move(result));
}

Result<Value> builtin_set_backend(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    std::string backend_name;
    if (const auto* s = args[0].as_string()) {
        backend_name = *s;
    } else if (const auto* sym = args[0].as_symbol()) {
        backend_name = sym->name;
    } else {
        return std::unexpected(type_error("set-backend!: argument must be a string or symbol"));
    }

    auto& registry = BackendRegistry::instance();
    if (!registry.set_active(backend_name)) {
        return std::unexpected(PMLException{
            ErrorCode::GeneralError, std::nullopt,
            "Unknown backend: " + backend_name, std::nullopt});
    }

    return make_nil_value();
}

Result<Value> builtin_current_backend(const std::vector<Value>&, Environment&) {
    auto& registry = BackendRegistry::instance();
    auto& backend = registry.active();
    auto info = backend.info();
    return Value(info.name);
}

Result<Value> builtin_backend_capabilities(const std::vector<Value>&, Environment&) {
    auto& registry = BackendRegistry::instance();
    auto& backend = registry.active();
    auto info = backend.info();
    auto caps = info.capabilities;

    std::vector<Value> result;

    if (has_capability(caps, BackendCap::RasterCPU))
        result.push_back(Value(Symbol("raster-cpu")));
    if (has_capability(caps, BackendCap::GPUAccel))
        result.push_back(Value(Symbol("gpu-accel")));
    if (has_capability(caps, BackendCap::Shaders))
        result.push_back(Value(Symbol("shaders")));
    if (has_capability(caps, BackendCap::VectorOutput))
        result.push_back(Value(Symbol("vector-output")));
    if (has_capability(caps, BackendCap::AnimationGIF))
        result.push_back(Value(Symbol("animation-gif")));
    if (has_capability(caps, BackendCap::FontRendering))
        result.push_back(Value(Symbol("font-rendering")));
    if (has_capability(caps, BackendCap::LoadPNG))
        result.push_back(Value(Symbol("load-png")));

    return make_list_value(std::move(result));
}

Result<Value> builtin_backend_available(const std::vector<Value>& args, Environment&) {
    if (args.size() != 1) {
        return std::unexpected(arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
    }

    std::string backend_name;
    if (const auto* s = args[0].as_string()) {
        backend_name = *s;
    } else if (const auto* sym = args[0].as_symbol()) {
        backend_name = sym->name;
    } else {
        return std::unexpected(type_error("backend-available?: argument must be a string or symbol"));
    }

    auto& registry = BackendRegistry::instance();
    auto backends = registry.available();

    for (const auto& info : backends) {
        if (info.name == backend_name) {
            return Value(true);
        }
    }

    return Value(false);
}

} // anonymous namespace

void register_backend_builtins(std::shared_ptr<Environment> env) {
    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    def("list-backends", builtin_list_backends);
    def("set-backend!", builtin_set_backend);
    def("current-backend", builtin_current_backend);
    def("backend-capabilities", builtin_backend_capabilities);
    def("backend-available?", builtin_backend_available);
}

} // namespace pml
