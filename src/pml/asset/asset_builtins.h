// ==========================================================================================================================================================================================================================================═
// Asset / Bitmap I/O builtins
// ==========================================================================================================================================================================================================================================═

#pragma once

#include <memory>

namespace pml {

class Environment;

/// Register image / bitmap / spritesheet builtins in the global environment.
///   image, load-image, bitmap-layer, load-spritesheet, asset-path?, clear-assets!
void register_asset_builtins(std::shared_ptr<Environment> env);

} // namespace pml
