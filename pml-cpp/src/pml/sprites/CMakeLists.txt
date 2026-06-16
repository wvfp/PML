add_library(pml_sprites
    palette.cpp
    style.cpp
)

target_include_directories(pml_sprites PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml_sprites PUBLIC pml_graphics pml_core pml_evaluator)
