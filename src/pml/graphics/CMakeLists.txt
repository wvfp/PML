add_library(pml_graphics
    transform.cpp
    params.cpp
    color.cpp
    objects.cpp
    canvas.cpp
    render.cpp
    rough.cpp
    rough_filler.cpp
    tilemap.cpp
    tileset.cpp
    perlin_noise.cpp
    polygon_perturb.cpp
)

target_include_directories(pml_graphics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../backend   # src/pml/backend/ (registry.h, backend.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../..        # src/pml/ (for pml/core/error.h via backend.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..     # src/ (for pml/api/context.h style paths)
    ${CMAKE_CURRENT_SOURCE_DIR}/../evaluator # src/pml/evaluator/ (environment.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../animation # src/pml/animation/ (timeline.h)
)
target_link_libraries(pml_graphics PUBLIC
    pml_core
    pml_backend
    pml_animation
)

# transform.cpp uses PML_USE_SKIA-gated SkMatrix include — give the graphics
# library access to the Skia include directory when Skia is enabled.
if(PML_USE_SKIA AND TARGET skia_headers)
    target_link_libraries(pml_graphics PUBLIC skia_headers)
endif()

# Optional backends: each target brings its own transitive deps (giflib,
# cairo, libpng, …) so the pml_graphics PUBLIC surface stays slim.
if(TARGET GIF::GIF)
    target_link_libraries(pml_graphics PUBLIC GIF::GIF)
endif()
if(TARGET PNG::PNG)
    target_link_libraries(pml_graphics PUBLIC PNG::PNG)
endif()
