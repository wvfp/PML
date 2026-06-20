add_library(pml_filter STATIC
    image_filter.cpp
    adjustments.cpp
    convolution.cpp
    layer_style.cpp
    filter_builtins.cpp
)

target_include_directories(pml_filter PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(pml_filter PUBLIC
    pml_backend
    pml_graphics
    pml_core
)
