// ==========================================================================================================================================================================================================================================═
// PML Module System — Module + ModuleLoader Implementation
// ==========================================================================================================================================================================================================================================═

#include "module_loader.h"

#include "pml/core/source_manager.h"
#include "environment.h"
#include "evaluator.h"
#include "expander.h"
#include "lexer.h"
#include "parser.h"

#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace pml {

namespace fs = std::filesystem;

// ==========================================================================================================================================================================================================================================═
// Module
// ==========================================================================================================================================================================================================================================═

Module::Module(std::string n, std::shared_ptr<Environment> e,
               std::unordered_set<std::string> ex)
    : name(std::move(n))
    , env(std::move(e))
    , exports(std::move(ex)) {}

Result<Value> Module::get(const std::string& symbol) const {
    if (exports.find(symbol) == exports.end()) {
        return std::unexpected(access_error(
            std::format("'{}' is not exported from module '{}'", symbol, name)));
    }
    return env->lookup(symbol);
}

bool Module::has(const std::string& symbol) const noexcept {
    return exports.find(symbol) != exports.end();
}

std::ostream& operator<<(std::ostream& os, const Module& mod) {
    // Collect sorted export names for display
    std::vector<std::string> sorted_exports(mod.exports.begin(), mod.exports.end());
    std::sort(sorted_exports.begin(), sorted_exports.end());

    os << "<Module '" << mod.name << "' exports=[";
    for (size_t i = 0; i < sorted_exports.size(); ++i) {
        if (i > 0) os << " ";
        os << sorted_exports[i];
    }
    os << "]>";
    return os;
}

// ==========================================================================================================================================================================================================================================═
// ModuleLoader
// ==========================================================================================================================================================================================================================================═

ModuleLoader::ModuleLoader(std::shared_ptr<Environment> global_env)
    : m_global_env(std::move(global_env)) {}

bool ModuleLoader::is_loaded(const std::string& path) const {
    return m_cache.find(path) != m_cache.end();
}

std::shared_ptr<Module> ModuleLoader::get_cached(
    const std::string& path) const {
    auto it = m_cache.find(path);
    if (it != m_cache.end()) {
        return it->second;
    }
    return nullptr;
}

bool ModuleLoader::is_available(
    const std::string& path, const std::string& from_file) const {
    auto resolved = resolve_path(path, from_file);
    return resolved.has_value();
}

std::vector<std::shared_ptr<Module>> ModuleLoader::loaded_modules() const {
    std::vector<std::shared_ptr<Module>> result;
    result.reserve(m_cache.size());
    for (const auto& [_, mod] : m_cache) {
        result.push_back(mod);
    }
    return result;
}

// ==========================================================================================================================================================================================================================================═
// resolve_path — search order:
//   1. path as-is (absolute or relative to cwd)
//   2. path + ".pml"
//   3. relative to from_file directory
//   4. relative to from_file directory + ".pml"
// ==========================================================================================================================================================================================================================================═

Result<std::string> ModuleLoader::resolve_path(
    const std::string& path, const std::string& from_file) const {

    // Helper lambda: check if a path is a regular file
    auto is_file = [](const fs::path& p) -> bool {
        std::error_code ec;
        bool result = fs::is_regular_file(p, ec);
        return result && !ec;
    };

    // 1. Try path as-is
    fs::path candidate(path);
    if (candidate.is_absolute()) {
        if (is_file(candidate)) {
            return candidate.string();
        }
        // Try with .pml extension
        if (candidate.extension() != ".pml") {
            auto with_ext = fs::path(path + ".pml");
            if (is_file(with_ext)) {
                return fs::absolute(with_ext).string();
            }
        }
    } else {
        // Relative path: resolve against current working directory
        auto abs_candidate = fs::absolute(candidate);
        if (is_file(abs_candidate)) {
            return abs_candidate.string();
        }
        // Try with .pml extension (cwd-relative)
        auto with_ext = fs::absolute(fs::path(path + ".pml"));
        if (is_file(with_ext)) {
            return with_ext.string();
        }
    }

    // 2. Try relative to importing file's directory
    if (!from_file.empty()) {
        fs::path from_path(from_file);
        auto base_dir = from_path.parent_path();

        // Try path relative to from_file directory
        auto rel_candidate = fs::absolute(base_dir / path);
        if (is_file(rel_candidate)) {
            return rel_candidate.string();
        }

        // Try with .pml extension
        if (fs::path(path).extension() != ".pml") {
            auto rel_with_ext = fs::absolute(base_dir / fs::path(path + ".pml"));
            if (is_file(rel_with_ext)) {
                return rel_with_ext.string();
            }
        }
    }

    // Not found
    std::string msg = std::format("Module not found: '{}'", path);
    if (!from_file.empty()) {
        msg += std::format(" (imported from {})", from_file);
    }
    return std::unexpected(import_error(std::move(msg)));
}

// ==========================================================================================================================================================================================================================================═
// load — main entry point: resolve, cache-check, circular dep detection, do_load
// ==========================================================================================================================================================================================================================================═

Result<std::shared_ptr<Module>> ModuleLoader::load(
    const std::string& path, const std::string& from_file) {

    auto resolved = resolve_path(path, from_file);
    if (!resolved) {
        return std::unexpected(resolved.error());
    }
    const std::string& resolved_path = *resolved;

    // Check cache — idempotent loading
    auto cache_it = m_cache.find(resolved_path);
    if (cache_it != m_cache.end()) {
        return cache_it->second;
    }

    // Circular dependency detection
    if (m_loading.find(resolved_path) != m_loading.end()) {
        return std::unexpected(circular_import_error(resolved_path));
    }

    // Mark as loading
    m_loading.insert(resolved_path);

    // Perform the actual load
    auto module_result = do_load(resolved_path, path);

    // Unmark regardless of success/failure
    m_loading.erase(resolved_path);

    if (!module_result) {
        return std::unexpected(module_result.error());
    }

    // Cache the module
    auto module = *module_result;
    m_cache[resolved_path] = module;
    return module;
}

// ==========================================================================================================================================================================================================================================═
// do_load — internal: read, lex, parse, expand, evaluate
// ==========================================================================================================================================================================================================================================═

Result<std::shared_ptr<Module>> ModuleLoader::do_load(
    const std::string& resolved_path, const std::string& original_path) {

    // 1. Read the file
    std::ifstream file_stream(resolved_path, std::ios::in | std::ios::binary);
    if (!file_stream) {
        return std::unexpected(import_error(
            std::format("Cannot open module file: '{}'", resolved_path)));
    }
    std::stringstream buffer;
    buffer << file_stream.rdbuf();
    std::string source = buffer.str();

    // Cache source for error snippet display across file boundaries.
    get_global_source_manager().load_source(resolved_path, source);

    // 2. Lex
    Lexer lexer(source, resolved_path);
    auto tokens_result = lexer.tokenize();
    if (!tokens_result) {
        return std::unexpected(tokens_result.error());
    }

    // 3. Parse
    Parser parser(std::move(*tokens_result), resolved_path);
    auto ast_result = parser.parse();
    if (!ast_result) {
        return std::unexpected(ast_result.error());
    }

    // 4. Create isolated module environment
    auto module_env = std::make_shared<Environment>(m_global_env);

    // 5. Set __source_file__ for relative import resolution within the module
    module_env->define("__source_file__", Value(std::string(resolved_path)));

    // 6. Macro expansion pass
    Expander expander(module_env);
    auto expanded_result = expander.expand_all(*ast_result);
    if (!expanded_result) {
        return std::unexpected(expanded_result.error());
    }

    // 7. Evaluate all expressions in module environment
    for (const auto& expr : *expanded_result) {
        auto eval_result = eval_to_value(expr, module_env);
        if (!eval_result) {
            return std::unexpected(eval_result.error());
        }
    }

    // 8. Extract module name from original path (stem)
    std::string module_name = fs::path(original_path).stem().string();

    // 9. Create module — exports were set by (provide ...) during evaluation
    auto module = std::make_shared<Module>(
        std::move(module_name), module_env, module_env->exports);

    return module;
}

// ==========================================================================================================================================================================================================================================═
// Global ModuleLoader accessor
// ==========================================================================================================================================================================================================================================═

std::shared_ptr<ModuleLoader> get_global_loader(std::shared_ptr<Environment> env) {
    // Walk to the root (top-level) environment
    auto root = std::move(env);
    while (root->parent) {
        root = root->parent;
    }

    // Last-resort fallback when no PMLContext is active (e.g., standalone tests).
    static std::shared_ptr<ModuleLoader> s_fallback;
    if (!s_fallback) {
        s_fallback = std::make_shared<ModuleLoader>(std::move(root));
    }
    return s_fallback;
}

}  // namespace pml
