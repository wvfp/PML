add_library(pml_animation
    easing.cpp
    timeline.cpp
)

target_include_directories(pml_animation PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../..        # src/pml/
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..     # src/ (for pml/api/context.h style paths)
)
target_link_libraries(pml_animation PUBLIC pml_graphics pml_evaluator pml_core)
target_link_libraries(pml_animation PRIVATE pml_skeleton)
