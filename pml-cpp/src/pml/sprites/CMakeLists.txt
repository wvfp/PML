add_library(pml_sprites
    palette.cpp
    style.cpp
    validator.cpp
    registry.cpp
    components/body.cpp
    components/head.cpp
    components/eyes.cpp
    components/mouth.cpp
    components/hair.cpp
    components/character.cpp
    components/outfit.cpp
    components/items.cpp
    components/ui_widgets.cpp
    components/scene_elements.cpp
)

target_include_directories(pml_sprites PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pml_sprites PUBLIC pml_graphics pml_core pml_evaluator)
