# PML Backend — ABC + Registry + NullBackend (always compiled)
add_library(pml_backend
    registry.cpp
    null_backend.cpp
    color_helpers.cpp
)

target_include_directories(pml_backend PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../..
)

target_link_libraries(pml_backend PUBLIC
    pml_core
)

# Cairo backend subdirectory (gated by PML_BUILD_CAIRO)
add_subdirectory(cairo)

# GIF export backend subdirectory (gated by PML_BUILD_GIF)
add_subdirectory(gif)
