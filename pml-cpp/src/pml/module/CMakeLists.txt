add_library(pml_module
    module_loader.cpp
    embedded_stdlib.cpp
)

target_include_directories(pml_module PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml_module PUBLIC pml_core pml_frontend pml_evaluator)
