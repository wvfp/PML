;; PML Standard Library — built-in color palettes
;;
;; Usage: (use-palette "ocean")
;;        (rect 0 0 40 40 :fill "$primary")
;;
;; Each palette defines semantic color keys for consistent styling.

;; ── Ocean — blue-gray tones, clean and professional ──────────────────
(palette "ocean"
  :primary   "#2c3e50"
  :secondary "#34495e"
  :accent    "#3498db"
  :bg        "#ecf0f1"
  :bg-alt    "#bdc3c7"
  :danger    "#e74c3c"
  :success   "#27ae60"
  :warning   "#f39c12"
  :text      "#2c3e50"
  :muted     "#95a5a6"
  :border    "#bdc3c7")

;; ── Forest — green and earth tones, natural feel ────────────────────
(palette "forest"
  :primary   "#2d5016"
  :secondary "#4a7c2e"
  :accent    "#8bc34a"
  :bg        "#f1f8e9"
  :bg-alt    "#dcedc8"
  :danger    "#c62828"
  :success   "#2e7d32"
  :warning   "#f9a825"
  :text      "#1b5e20"
  :muted     "#9e9d24"
  :border    "#aed581")

;; ── Sunset — warm orange and purple, dramatic ───────────────────────
(palette "sunset"
  :primary   "#e65100"
  :secondary "#ff6f00"
  :accent    "#ffab00"
  :bg        "#fff3e0"
  :bg-alt    "#ffe0b2"
  :danger    "#d50000"
  :success   "#2e7d32"
  :warning   "#ff6d00"
  :text      "#3e2723"
  :muted     "#bf8f6b"
  :border    "#ffcc80")

;; ── Retro — bright, saturated, pixel-game style ─────────────────────
(palette "retro"
  :primary   "#212529"
  :secondary "#495057"
  :accent    "#ff6b35"
  :bg        "#f8f9fa"
  :bg-alt    "#e9ecef"
  :danger    "#e63946"
  :success   "#2a9d8f"
  :warning   "#e9c46a"
  :text      "#212529"
  :muted     "#adb5bd"
  :border    "#dee2e6")

;; ── Mono — grayscale, for wireframes and minimal design ─────────────
(palette "mono"
  :primary   "#000000"
  :secondary "#333333"
  :accent    "#666666"
  :bg        "#f5f5f5"
  :bg-alt    "#e0e0e0"
  :danger    "#999999"
  :success   "#666666"
  :warning   "#cccccc"
  :text      "#000000"
  :muted     "#bdbdbd"
  :border    "#e0e0e0")

;; ── Game-UI — high contrast, vivid, for game interfaces ─────────────
(palette "game-ui"
  :primary   "#1a1a2e"
  :secondary "#16213e"
  :accent    "#e94560"
  :bg        "#0f3460"
  :bg-alt    "#1a1a40"
  :danger    "#ff2e63"
  :success   "#00b894"
  :warning   "#fdcb6e"
  :text      "#ffffff"
  :muted     "#636e72"
  :border    "#2d3436")

;; ── Pastel — soft colors, suitable for character art ────────────────
(palette "pastel"
  :primary   "#5c6bc0"
  :secondary "#7986cb"
  :accent    "#f06292"
  :bg        "#fafafa"
  :bg-alt    "#f5f5f5"
  :danger    "#ef5350"
  :success   "#66bb6a"
  :warning   "#ffee58"
  :text      "#37474f"
  :muted     "#b0bec5"
  :border    "#e0e0e0")

;; ── Dark — dark mode friendly ───────────────────────────────────────
(palette "dark"
  :primary   "#e0e0e0"
  :secondary "#9e9e9e"
  :accent    "#64b5f6"
  :bg        "#121212"
  :bg-alt    "#1e1e1e"
  :danger    "#ef5350"
  :success   "#66bb6a"
  :warning   "#ffd54f"
  :text      "#e0e0e0"
  :muted     "#616161"
  :border    "#333333")

(provide palettes)
