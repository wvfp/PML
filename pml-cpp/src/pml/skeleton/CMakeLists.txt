add_library(pml_skeleton
    skeleton.cpp
    ik_ccd.cpp
    ik_fabrik.cpp
)

target_include_directories(pml_skeleton PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml_skeleton PUBLIC pml_graphics pml_core pml_evaluator)
