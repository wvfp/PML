#pragma once

#include "pml/core/types.h"

#include <memory>
#include <string>

namespace pml {

class Environment;

void register_layer_builtins(std::shared_ptr<Environment> env);

[[nodiscard]] Result<std::string> render_composition_to_disk(
    const Composition& comp, const std::string& output_dir,
    const std::string& filename = "");

} // namespace pml
