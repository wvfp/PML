add_library(pml_asset STATIC
    asset_loader.cpp
    asset_cache.cpp
    asset_builtins.cpp
)

target_link_libraries(pml_asset PUBLIC
    pml_backend
    pml_core
    pml_graphics
    pml_layer
    nlohmann_json::nlohmann_json
)

target_include_directories(pml_asset PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)
