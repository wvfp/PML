add_library(pml_animation
    easing.cpp
    timeline.cpp
)

target_include_directories(pml_animation PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pml_animation PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../..)
target_link_libraries(pml_animation PUBLIC pml_graphics pml_evaluator pml_core)
target_link_libraries(pml_animation PRIVATE pml_skeleton)
