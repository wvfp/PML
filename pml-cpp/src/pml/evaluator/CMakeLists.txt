add_library(pml_evaluator
    environment.cpp
    evaluator.cpp
    builtins.cpp
    backend_builtins.cpp
    shader_builtins.cpp
    transform_builtins.cpp
    canvas_builtins.cpp
)

target_include_directories(pml_evaluator PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pml_evaluator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../module)
target_include_directories(pml_evaluator PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_include_directories(pml_evaluator PRIVATE ${CAIRO_INCLUDE_DIRS})
target_link_libraries(pml_evaluator PUBLIC pml_core pml_frontend pml_module)
