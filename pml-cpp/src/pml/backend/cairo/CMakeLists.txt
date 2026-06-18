# Cairo render backend — gated by PML_BUILD_CAIRO (OFF by default: cairo on
# Windows MSVC requires pixman/zlib/freetype and is fragile). Set
# PML_BUILD_CAIRO=ON and provide those deps yourself to enable.
if(NOT PML_BUILD_CAIRO)
    return()
endif()

if(NOT TARGET Cairo::Cairo)
    message(FATAL_ERROR
        "PML_BUILD_CAIRO=ON but Cairo::Cairo target is not defined. "
        "Cairo needs to be available before configuring.")
endif()

add_library(pml_backend_cairo
    cairo_backend.cpp
)

target_include_directories(pml_backend_cairo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
)
target_link_libraries(pml_backend_cairo PUBLIC
    pml_backend
    pml_graphics
    Cairo::Cairo
)
