# Skia backend — gated by PML_BUILD_SKIA (ON by default).
# Skia is consumed as a pre-built static library; the imported targets
# (skia::skia, skia::skcms, skia::sksg, …) are set up by the top-level
# CMakeLists.txt using PML_SKIA_DIR / PML_SKIA_OUT.
if(NOT PML_BUILD_SKIA)
    return()
endif()

if(NOT TARGET pml::skia_deps)
    message(FATAL_ERROR
        "PML_BUILD_SKIA=ON but pml::skia_deps interface target is missing. "
        "Set PML_SKIA_DIR and PML_SKIA_OUT to point at a pre-built Skia.")
endif()

# Skeleton implementation — flesh out in Task 12. The target is registered
# so the rest of the build can link against it without further changes.
add_library(pml_backend_skia
    skia_backend.cpp
)

target_include_directories(pml_backend_skia PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
target_link_libraries(pml_backend_skia PUBLIC
    pml_backend
    pml_graphics
    pml_asset
    pml::skia_deps
)
