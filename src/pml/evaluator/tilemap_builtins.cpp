// ═══════════════════════════════════════════════════════════════════════════════
// PML Tilemap Builtins — Tile-based game map data management
// ───────────────────────────────────────────────────────────────────────────────
// Registers define-tileset, make-tilemap, tilemap-set! built-in procedures
// for the data-management layer of tile-based game maps.
//
// Builtins:
//   (define-tileset name :tile-size N :tiles '((id name graphic ...) ...))
//     — register a named tileset with tile types
//   (make-tilemap tileset-name cols rows [:projection 'orthogonal|'isometric] [:layers 1])
//     — create a named tilemap
//   (tilemap-set! tilemap-name layer col row tile-id)
//     — set a tile in the tilemap
// ═══════════════════════════════════════════════════════════════════════════════

#include "tilemap_builtins.h"

#include "pml/graphics/tileset.h"
#include "pml/graphics/tilemap.h"
#include "pml/graphics/graphic_object.h"
#include "environment.h"
#include "error.h"
#include "evaluator.h"
#include "kwargs.h"
#include "types.h"

#include <algorithm>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace pml {

// Bring kwargs helpers into scope for the entire file.
using pml::kwargs::value_to_opt_string;
using pml::kwargs::parse_kwargs;
using pml::kwargs::kw_int;

namespace {

// ═══════════════════════════════════════════════════════════════════════════════
// value_to_expr — Convert a runtime Value back to an AST Expr for evaluation
// ───────────────────────────────────────────────────────────────────────────────
// When the user passes `'((id name (rect ...)))` via a kwarg, the graphic
// expression arrives as a quoted ValueList.  To evaluate it we must convert
// it back to an Expr and call eval_to_value.
// ═══════════════════════════════════════════════════════════════════════════════

Expr value_to_expr(const Value& v) {
    if (v.is_nil()) return nullptr;
    if (v.is_int()) return v.int_val();
    if (v.is_double()) return v.double_val();
    if (v.is_bool()) return v.bool_val();
    if (const auto* s = v.as_string()) return *s;
    if (const auto* sym = v.as_symbol()) return *sym;
    if (const auto* kw = v.as_keyword()) return *kw;
    if (const auto* lst = v.as_list()) {
        auto elems = std::vector<Expr>{};
        elems.reserve(lst->get()->elements.size());
        for (const auto& elem : lst->get()->elements) {
            elems.push_back(value_to_expr(elem));
        }
        return make_list_heap(std::move(elems));
    }
    // For complex types (GraphicObject, etc.) — return nil (evaluates to nil)
    return nullptr;
}

/// Evaluate a Value as a PML expression in the given environment.
/// Converts the Value to an Expr first, then evaluates it.
Result<Value> eval_value_as_expr(const Value& v, Environment& env) {
    Expr expr = value_to_expr(v);
    return eval_to_value(expr, env.shared_from_this());
}

// ═══════════════════════════════════════════════════════════════════════════════
// extract_tile_entry — Convert a ValueList entry to a TileType
// ───────────────────────────────────────────────────────────────────────────────
// A tile entry is a list: (id name [graphic-expr [detail-expr]])
// Returns an error if the entry is malformed.
// ═══════════════════════════════════════════════════════════════════════════════

Result<TileType> extract_tile_entry(const Value& entry_val, Environment& env) {
    const auto* entry_list = entry_val.as_list();
    if (!entry_list || !*entry_list) {
        return std::unexpected(
            type_error("define-tileset: each tile entry must be a list"));
    }

    const auto& elements = (*entry_list)->elements;

    // Need at least 2 elements: (id name)
    if (elements.size() < 2) {
        return std::unexpected(
            type_error("define-tileset: each tile entry needs at least (id name)"));
    }

    // Extract id (first element, must be int)
    if (!elements[0].is_int()) {
        return std::unexpected(
            type_error("define-tileset: tile id must be an integer"));
    }
    int id = static_cast<int>(elements[0].int_val());

    // Extract name (second element, string or symbol)
    auto name_opt = value_to_opt_string(elements[1]);
    if (!name_opt) {
        return std::unexpected(
            type_error("define-tileset: tile name must be a string or symbol"));
    }

    // If there's a third element, it's the graphic expression
    GraphicObject graphic;
    if (elements.size() >= 3) {
        auto result = eval_value_as_expr(elements[2], env);
        if (!result) {
            return std::unexpected(std::move(result.error()));
        }
        const auto* go = result->as_graphic_object();
        if (go && *go) {
            graphic = **go;
        } else {
            // The expression evaluated to something that isn't a GraphicObject.
            // That's OK — we store a default GraphicObject as a fallback.
        }
    }

    // If there's a fourth element, it's the detail overlay expression
    std::optional<GraphicObject> detail;
    if (elements.size() >= 4) {
        auto result = eval_value_as_expr(elements[3], env);
        if (!result) {
            return std::unexpected(std::move(result.error()));
        }
        const auto* go = result->as_graphic_object();
        if (go && *go) {
            detail = **go;
        }
    }

    TileType tt;
    tt.id = id;
    tt.name = *name_opt;
    tt.graphic = std::move(graphic);
    tt.detail = std::move(detail);
    return tt;
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
// Builtin Registration
// ═══════════════════════════════════════════════════════════════════════════════

void register_tilemap_builtins(std::shared_ptr<Environment> env) {
    if (!env) return;

    auto def = [&](const std::string& name, BuiltinProcedure::BuiltinFn fn) {
        auto proc = std::make_shared<BuiltinProcedure>(name, std::move(fn));
        env->define(name, Value(proc));
    };

    // ── (define-tileset name :tile-size N :tiles '((id name graphic ...) ...)) ──
    // Register a named tileset with tile types.
    //
    // Positional args: name (symbol or string)
    // Kwargs: :tile-size (int, default 32), :tiles (list of tile entries)
    //
    // Each tile entry: (id name [graphic-expr [detail-expr]])
    //   - id: integer tile index
    //   - name: string or symbol label
    //   - graphic-expr: PML expression that evaluates to a GraphicObject
    //   - detail-expr: optional overlay expression
    //
    // Returns the tileset name (as string) for chaining.
    def("define-tileset", [](const std::vector<Value>& args,
                              Environment& env) -> Result<Value> {
        if (args.empty()) {
            return std::unexpected(
                arity_error(SourceLocation{}, 1, static_cast<int>(args.size())));
        }

        // First positional arg: tileset name
        auto name_opt = value_to_opt_string(args[0]);
        if (!name_opt) {
            return std::unexpected(
                type_error("define-tileset: first argument must be a string or symbol"));
        }

        // Parse kwargs starting after position 0
        auto kwargs = parse_kwargs(args, 1);

        int tile_size = kw_int(kwargs, "tile-size", 32);

        // Extract :tiles kwarg
        std::vector<TileType> tile_types;
        if (auto it = kwargs.find("tiles"); it != kwargs.end()) {
            const auto* tiles_list = it->second.as_list();
            if (!tiles_list || !*tiles_list) {
                return std::unexpected(
                    type_error("define-tileset: :tiles must be a list"));
            }

            const auto& entries = (*tiles_list)->elements;
            tile_types.reserve(entries.size());

            for (size_t i = 0; i < entries.size(); ++i) {
                auto tt = extract_tile_entry(entries[i], env);
                if (!tt) {
                    return std::unexpected(
                        type_error(std::format("define-tileset: tile entry {}: {}",
                                               i, tt.error().message)));
                }
                tile_types.push_back(std::move(*tt));
            }
        }

        // Register with TilesetManager
        TilesetManager::instance().register_tileset(*name_opt, tile_types, tile_size);

        return Value(*name_opt);
    });

    // ── (make-tilemap tileset-name cols rows [:projection 'orthogonal|'isometric] [:layers 1]) ──
    // Create a named tilemap and store it in TilemapManager.
    //
    // Positional args: tileset-name, cols, rows
    // Kwargs: :projection ('orthogonal or 'isometric), :layers (int, default 1)
    //
    // The tilemap name is the same as the tileset name for chaining.
    // Returns the tilemap name (as string).
    def("make-tilemap", [](const std::vector<Value>& args,
                            Environment& env) -> Result<Value> {
        if (args.size() < 3) {
            return std::unexpected(
                arity_error(SourceLocation{}, 3, static_cast<int>(args.size())));
        }

        // First positional: tileset name
        auto ts_name_opt = value_to_opt_string(args[0]);
        if (!ts_name_opt) {
            return std::unexpected(
                type_error("make-tilemap: first argument must be a string or symbol (tileset name)"));
        }

        // Second positional: cols
        if (!args[1].is_int()) {
            return std::unexpected(
                type_error("make-tilemap: cols must be an integer"));
        }
        int cols = static_cast<int>(args[1].int_val());
        if (cols <= 0) {
            return std::unexpected(
                type_error("make-tilemap: cols must be positive"));
        }

        // Third positional: rows
        if (!args[2].is_int()) {
            return std::unexpected(
                type_error("make-tilemap: rows must be an integer"));
        }
        int rows = static_cast<int>(args[2].int_val());
        if (rows <= 0) {
            return std::unexpected(
                type_error("make-tilemap: rows must be positive"));
        }

        // Parse kwargs starting at position 3
        auto kwargs = parse_kwargs(args, 3);

        // :projection kwarg
        Projection projection = Projection::Orthogonal;
        if (auto it = kwargs.find("projection"); it != kwargs.end()) {
            auto proj_str = value_to_opt_string(it->second);
            if (proj_str && *proj_str == "isometric") {
                projection = Projection::Isometric;
            }
        }

        // :layers kwarg
        int num_layers = kw_int(kwargs, "layers", 1);
        if (num_layers < 1) {
            num_layers = 1;
        }

        // Verify the tileset exists (look it up)
        const Tileset* ts = TilesetManager::instance().lookup_tileset(*ts_name_opt);
        if (!ts) {
            return std::unexpected(
                type_error(std::format("make-tilemap: unknown tileset '{}'", *ts_name_opt)));
        }

        // Create layers
        std::vector<TilemapLayer> layers;
        layers.reserve(static_cast<size_t>(num_layers));
        for (int i = 0; i < num_layers; ++i) {
            layers.push_back(add_layer(cols, rows, true));
        }

        // Register the tilemap with name = tileset_name
        TilemapManager::instance().create_tilemap(
            *ts_name_opt, *ts_name_opt, cols, rows, projection, std::move(layers));

        return Value(*ts_name_opt);
    });

    // ── (tilemap-set! tilemap-name layer col row tile-id) ────────────────────
    // Set a tile in the tilemap.  Returns #t on success.
    //
    // All positionals — no kwargs.
    // Out-of-bounds accesses silently do nothing (matching add_layer / set_tile).
    def("tilemap-set!", [](const std::vector<Value>& args,
                            Environment& /*env*/) -> Result<Value> {
        if (args.size() != 5) {
            return std::unexpected(
                arity_error(SourceLocation{}, 5, static_cast<int>(args.size())));
        }

        // tilemap-name
        auto tm_name_opt = value_to_opt_string(args[0]);
        if (!tm_name_opt) {
            return std::unexpected(
                type_error("tilemap-set!: first argument must be a string or symbol (tilemap name)"));
        }

        // layer
        if (!args[1].is_int()) {
            return std::unexpected(
                type_error("tilemap-set!: layer must be an integer"));
        }
        int layer = static_cast<int>(args[1].int_val());

        // col
        if (!args[2].is_int()) {
            return std::unexpected(
                type_error("tilemap-set!: col must be an integer"));
        }
        int col = static_cast<int>(args[2].int_val());

        // row
        if (!args[3].is_int()) {
            return std::unexpected(
                type_error("tilemap-set!: row must be an integer"));
        }
        int row = static_cast<int>(args[3].int_val());

        // tile-id
        if (!args[4].is_int()) {
            return std::unexpected(
                type_error("tilemap-set!: tile-id must be an integer"));
        }
        int tile_id = static_cast<int>(args[4].int_val());

        // Look up tilemap
        Tilemap* tm = TilemapManager::instance().lookup_tilemap(*tm_name_opt);
        if (!tm) {
            return std::unexpected(
                type_error(std::format("tilemap-set!: unknown tilemap '{}'", *tm_name_opt)));
        }

        // Validate layer index
        if (layer < 0 || static_cast<size_t>(layer) >= tm->layers.size()) {
            // Out-of-range layer: silently no-op (matching set_tile style)
            return Value(true);
        }

        // Set the tile (set_tile handles bounds checking)
        set_tile(tm->layers[static_cast<size_t>(layer)], col, row, tile_id);

        return Value(true);
    });
}

} // namespace pml
