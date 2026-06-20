add_library(pml_graphics3d STATIC
    mat4.cpp
    camera3d.cpp
    mesh3d.cpp
    primitive_factory.cpp
    builtins_3d.cpp
)

target_include_directories(pml_graphics3d PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(pml_graphics3d PUBLIC pml_core pml_graphics)
