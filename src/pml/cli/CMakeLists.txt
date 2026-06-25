add_executable(pml
    main.cpp
    repl.cpp
    cli_errors.cpp
    cli_watch.cpp
    cli_execute.cpp
    # Directly compile texture/triangulation sources into pml.exe
    # to avoid MSVC static library incremental archiving issues.
    ${CMAKE_SOURCE_DIR}/src/pml/graphics/triangulation.cpp
    ${CMAKE_SOURCE_DIR}/src/pml/graphics/uv_solver.cpp
    ${CMAKE_SOURCE_DIR}/src/pml/backend/skia/textured_draw.cpp
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

# poly2tri for triangulation
if(TARGET poly2tri)
    target_link_libraries(pml PRIVATE poly2tri)
endif()

# Force inclusion of backend static initializers by wrapping with whole-archive.
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
    foreach(_b IN LISTS _pml_present_backends)
        if(MSVC)
            target_link_options(pml PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:${_b}>")
        else()
            target_link_options(pml PRIVATE
                -Wl,--whole-archive "$<TARGET_FILE:${_b}>" -Wl,--no-whole-archive
            )
        endif()
    endforeach()
endif()

# Install target
install(TARGETS pml DESTINATION bin)
