# GIF export backend — always compiled (giflib is a required dependency)
add_library(pml_backend_gif
    gif_exporter.cpp
)

target_include_directories(pml_backend_gif PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../..
    ${GIFLIB_INCLUDE_DIRS}
)

target_link_libraries(pml_backend_gif PUBLIC
    ${GIFLIB_LIBRARIES}
)

target_compile_options(pml_backend_gif PUBLIC
    ${GIFLIB_CFLAGS_OTHER}
)
