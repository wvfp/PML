add_library(pml_layer STATIC
    blend_mode.cpp
    blend_mode_skia.cpp
    layer.cpp
    composition.cpp
    compositor.cpp
    spritesheet.cpp
    aseprite_writer.cpp
    layer_builtins.cpp
)

target_include_directories(pml_layer PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(pml_layer PUBLIC
    pml_core
    pml_graphics
    pml_backend
    pml_filter
    nlohmann_json::nlohmann_json
)
