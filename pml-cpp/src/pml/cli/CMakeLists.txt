add_executable(pml
    main.cpp
)

target_include_directories(pml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml PRIVATE
    pml_core
    pml_frontend
    pml_evaluator
    pml_graphics
    pml_sprites
    pml_animation
    pml_skeleton
    pml_module
)

# Install target
install(TARGETS pml DESTINATION bin)
