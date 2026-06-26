#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Module System — Module + ModuleLoader with caching and circular dep
// detection.
//
// Matches Python pml/module_loader.py exactly.
// ==========================================================================================================================================================================================================================================═

#include "error.h"
#include "types.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace pml {

class Environment;

// ==========================================================================================================================================================================================================================================═
// Module — a loaded PML module with its environment and export set
// ==========================================================================================================================================================================================================================================═

class Module : public std::enable_shared_from_this<Module> {
public:
    std::string name;
    std::shared_ptr<Environment> env;
    std::unordered_set<std::string> exports;

    Module(std::string n, std::shared_ptr<Environment> e,
           std::unordered_set<std::string> ex = {});
    ~Module() = default;

    /// Get an exported symbol's value. Returns AccessError if not exported.
    [[nodiscard]] Result<Value> get(const std::string& symbol) const;

    /// Check if a symbol is exported from this module.
    [[nodiscard]] bool has(const std::string& symbol) const noexcept;

    friend std::ostream& operator<<(std::ostream& os, const Module& mod);
};

// ==========================================================================================================================================================================================================================================═
// ModuleLoader — loads PML modules with caching and circular dependency
// detection
// ==========================================================================================================================================================================================================================================═

class ModuleLoader : public std::enable_shared_from_this<ModuleLoader> {
public:
    explicit ModuleLoader(std::shared_ptr<Environment> global_env);

    /// Load a module, returning the cached version if already loaded.
    /// @param path      The module path (e.g., "lib/math.pml")
    /// @param from_file The file doing the importing (for relative resolution)
    [[nodiscard]] Result<std::shared_ptr<Module>> load(
        const std::string& path, const std::string& from_file = "");

    /// Check if a module path is already loaded (cached).
    [[nodiscard]] bool is_loaded(const std::string& path) const;

    /// Get a cached module by its resolved absolute path, or nullptr.
    [[nodiscard]] std::shared_ptr<Module> get_cached(
        const std::string& path) const;

    /// Check if a module path can be resolved to an existing file without
    /// actually loading it.
    [[nodiscard]] bool is_available(
        const std::string& path, const std::string& from_file = "") const;

    /// Return all currently loaded modules.
    [[nodiscard]] std::vector<std::shared_ptr<Module>> loaded_modules() const;

private:
    std::shared_ptr<Environment> m_global_env;
    std::unordered_map<std::string, std::shared_ptr<Module>> m_cache;
    std::unordered_set<std::string> m_loading;  // circular dep detection

    /// Resolve a module path to an absolute file path.
    /// Tries: path as-is, path + ".pml", relative to from_file.
    /// Returns ImportError if not found.
    [[nodiscard]] Result<std::string> resolve_path(
        const std::string& path, const std::string& from_file) const;

    /// Internal: parse, expand macros, and evaluate a module file.
    [[nodiscard]] Result<std::shared_ptr<Module>> do_load(
        const std::string& resolved_path, const std::string& original_path);
};

// ==========================================================================================================================================================================================================================================═
// Global ModuleLoader accessor
// ==========================================================================================================================================================================================================================================═

/// Get or create the global ModuleLoader instance.
/// On first call, walks `env` to find the root environment and creates the
/// loader. Subsequent calls return the cached instance (the env argument is
/// ignored).
[[nodiscard]] std::shared_ptr<ModuleLoader> get_global_loader(
    std::shared_ptr<Environment> env);

}  // namespace pml
