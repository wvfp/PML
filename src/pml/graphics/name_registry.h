#pragma once

// ==========================================================================================================================================================================================================================================═
// PML Name Registry — map string names → GraphicObject
// ==========================================================================================================================================================================================================================================═
//
// Allows PML scripts to assign names to GraphicObjects and retrieve them
// later by name for mutation or reuse.  The registry is cleared on each
// PMLRuntime::reset().
// ==========================================================================================================================================================================================================================================═

#include <optional>
#include <string>
#include <unordered_map>

#include "pml/graphics/objects.h"

namespace pml {

class NameRegistry {
public:
    /// Register or replace a named object.
    void register_object(const std::string& name, const GraphicObject& obj) {
        objects_[name] = obj;
    }

    /// Look up a named object.  Returns std::nullopt if not found.
    [[nodiscard]] std::optional<GraphicObject> find(const std::string& name) const {
        auto it = objects_.find(name);
        if (it == objects_.end()) return std::nullopt;
        return it->second;
    }

    /// Remove a named object.  Returns true if it existed.
    bool remove(const std::string& name) {
        return objects_.erase(name) > 0;
    }

    /// Replace a named object.  Returns false if the name doesn't exist.
    bool replace(const std::string& name, const GraphicObject& obj) {
        auto it = objects_.find(name);
        if (it == objects_.end()) return false;
        it->second = obj;
        return true;
    }

    /// Clear all registered objects.
    void clear() {
        objects_.clear();
    }

private:
    std::unordered_map<std::string, GraphicObject> objects_;
};

} // namespace pml
