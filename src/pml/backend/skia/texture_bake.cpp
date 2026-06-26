// ==========================================================================================================================================================================================================================================═
// PML Texture Bake — thin stub, implementation merged into textured_draw.cpp
// ==========================================================================================================================================================================================================================================═
//
// This file exists so that pml/graphics/texture_bake.h can be included from
// other translation units (texture_map_builtins.cpp, draw_object dispatch, etc.)
// without circular dependency.  The actual implementation lives in
// textured_draw.cpp and is linked directly into the target.
//
// Do NOT add implementation here.  Keep this file empty (no symbols).
