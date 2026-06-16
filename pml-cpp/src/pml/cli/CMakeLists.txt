add_executable(pml
    main.cpp
    repl.cpp
)

target_include_directories(pml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml PRIVATE
    pml_api
    pml_core
    pml_frontend
    pml_evaluator
    pml_graphics
    pml_sprites
    pml_animation
    pml_skeleton
    pml_module
)

# Force inclusion of backend static initializers by wrapping with whole-archive
target_link_libraries(pml PRIVATE
    -Wl,--whole-archive
    pml_backend_cairo
    pml_backend_gif
    -Wl,--no-whole-archive
)

# Install target
install(TARGETS pml DESTINATION bin)
