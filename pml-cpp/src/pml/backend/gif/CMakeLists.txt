# GIF export backend — gated by PML_BUILD_GIF (ON by default).
# giflib is fetched via the pml_third_party helper in the top-level
# CMakeLists.txt (git-cloned from upstream if third_party/giflib is missing).
if(NOT PML_BUILD_GIF)
    return()
endif()

if(NOT TARGET GIF::GIF)
    message(FATAL_ERROR
        "PML_BUILD_GIF=ON but GIF::GIF target is not defined. "
        "giflib needs to be available before configuring.")
endif()

add_library(pml_backend_gif
    gif_exporter.cpp
)

target_include_directories(pml_backend_gif PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
target_link_libraries(pml_backend_gif PUBLIC
    GIF::GIF
)
