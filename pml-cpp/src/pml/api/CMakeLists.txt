# PML API — PMLRuntime facade (task 30)
add_library(pml_api
    api.cpp
)

target_include_directories(pml_api PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..    # src/pml/ — for internal cross-module includes
    ${CMAKE_CURRENT_SOURCE_DIR}/../.. # src/ — for #include "pml/api/api.h" style paths
)

target_link_libraries(pml_api PUBLIC
    pml_core           # types.h, error.h — exposed in public header
)

target_link_libraries(pml_api PRIVATE
    pml_frontend       # Lexer, Parser, Expander
    pml_evaluator      # evaluate(), Environment, BuiltinProcedure
    pml_graphics       # render, canvas, objects
    pml_backend        # BackendRegistry (indirect via graphics)
    pml_sprites        # StyleRegistry, PaletteManager
    pml_animation      # Timeline, easing
    pml_skeleton       # SkeletonTemplate, SkeletonInstance, IK
    pml_module         # ModuleLoader, Module
)
