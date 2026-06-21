add_executable(pml
    main.cpp
    repl.cpp
    cli_errors.cpp
    cli_watch.cpp
    cli_execute.cpp
)

target_include_directories(pml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml PRIVATE
    pml_api
    pml_core
    pml_frontend
    pml_evaluator
    pml_graphics
    pml_layer
    pml_sprites
    pml_animation
    pml_skeleton
    pml_module
)

# Force inclusion of backend static initializers by wrapping with whole-archive.
# Each backend target is optional (gated by PML_BUILD_GIF / PML_BUILD_SKIA in
# the top-level CMakeLists.txt).
set(_pml_optional_backends
    pml_backend_gif
    pml_backend_skia
)
set(_pml_present_backends "")
foreach(_b IN LISTS _pml_optional_backends)
    if(TARGET ${_b})
        list(APPEND _pml_present_backends ${_b})
    endif()
endforeach()
if(_pml_present_backends)
    target_link_libraries(pml PRIVATE ${_pml_present_backends})
    if(MSVC)
        foreach(_b IN LISTS _pml_present_backends)
            target_link_options(pml PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${_b}>")
        endforeach()
    else()
        target_link_options(pml PRIVATE
            -Wl,--whole-archive
            ${_pml_present_backends}
            -Wl,--no-whole-archive
        )
    endif()
endif()

# Install target
install(TARGETS pml DESTINATION bin)
