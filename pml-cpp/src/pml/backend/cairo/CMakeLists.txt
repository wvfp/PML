# Cairo render backend — always compiled (Cairo is a required dependency)
add_library(pml_backend_cairo
    cairo_backend.cpp
)

target_include_directories(pml_backend_cairo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${CAIRO_INCLUDE_DIRS}
    ${PNG_INCLUDE_DIRS}
)

target_link_libraries(pml_backend_cairo PUBLIC
    pml_backend
    pml_graphics
    ${CAIRO_LIBRARIES}
    ${PNG_LIBRARIES}
)

target_compile_options(pml_backend_cairo PUBLIC
    ${CAIRO_CFLAGS_OTHER}
    ${PNG_CFLAGS_OTHER}
)
