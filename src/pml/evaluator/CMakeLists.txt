add_library(pml_evaluator
    environment.cpp
    evaluator.cpp
    builtins.cpp
    builtins_arithmetic.cpp
    builtins_io.cpp
    builtins_list.cpp
    builtins_string.cpp
    builtins_predicates.cpp
    builtins_containers.cpp
    builtins_threading.cpp
    builtins_polymorphic.cpp
    backend_builtins.cpp
    shader_builtins.cpp
    transform_builtins.cpp
    canvas_builtins.cpp
    shape_builtins.cpp
    object_style_builtins.cpp
    color_builtins.cpp
    gradient_builtins.cpp
    clip_builtins.cpp
    tilemap_builtins.cpp
    render_channels_builtins.cpp
    multi_texture_builtins.cpp
    name_builtins.cpp
    texture_builtins.cpp
    texture_map_builtins.cpp
    perturb_builtins.cpp
    macro_expansion.cpp
)

target_include_directories(pml_evaluator PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pml_evaluator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../module)
target_include_directories(pml_evaluator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(pml_evaluator PUBLIC pml_core pml_frontend pml_module pml_backend pml_graphics)

