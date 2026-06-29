add_executable(pml
    main.cpp
    repl.cpp
    cli_errors.cpp
    cli_watch.cpp
    cli_execute.cpp
)

target_include_directories(pml PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(pml PRIVATE "${CMAKE_SOURCE_DIR}/third_party/poly2tri")

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

if(TARGET poly2tri)
    target_link_libraries(pml PRIVATE poly2tri)
endif()

# Force inclusion of backend static initializers by wrapping with whole-archive.
# pml_backend contains null_backend.cpp (static registration) — must be
# whole-archived or the linker skips it since nothing references its symbols.
set(_pml_optional_backends
    pml_backend
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
        # Collect ALL backend .a files into a single --whole-archive block.
        # A single pair avoids Ninja combining separate -Wl, flags incorrectly.
        set(_wa_backend_files "")
        foreach(_b IN LISTS _pml_present_backends)
            list(APPEND _wa_backend_files "$<TARGET_FILE:${_b}>")
        endforeach()
        target_link_options(pml PRIVATE
            -Wl,--whole-archive ${_wa_backend_files} -Wl,--no-whole-archive)
    endif()
endif()

install(TARGETS pml DESTINATION bin)
