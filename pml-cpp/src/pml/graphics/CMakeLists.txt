add_library(pml_graphics
    transform.cpp
    objects.cpp
    canvas.cpp
    render.cpp
)

target_include_directories(pml_graphics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../backend   # src/pml/backend/ (registry.h, backend.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../..        # src/pml/ (for pml/core/error.h via backend.h)
    ${CMAKE_CURRENT_SOURCE_DIR}/../evaluator # src/pml/evaluator/ (environment.h)
    ${GIFLIB_INCLUDE_DIRS}
    ${CAIRO_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
)
target_link_libraries(pml_graphics PUBLIC
    pml_core
    pml_backend
    ${GIFLIB_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PNG_LIBRARIES}
)
target_compile_options(pml_graphics PUBLIC ${GIFLIB_CFLAGS_OTHER} ${CAIRO_CFLAGS_OTHER} ${PNG_CFLAGS_OTHER})
