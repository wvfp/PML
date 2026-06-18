"""PML project management — pml.toml project config, rendering, scaffolding."""

from __future__ import annotations

import concurrent.futures
import hashlib
import json
import os
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any

from pml.evaluator import _get_module_loader
from pml.graphics.canvas import get_current_canvas
from pml.graphics.render import _output_dir, _render, _render_spritesheet, set_output_dir
from pml.repl import run_file, create_global_env

try:
    import tomllib
except ImportError:
    import tomli as tomllib


# ======================================================================
# Data models
# ======================================================================


@dataclass
class RenderTarget:
    src: str
    out: str | None = None
    fps: int | None = None


@dataclass
class SpritesheetTarget:
    name: str
    sources: list[str] = field(default_factory=list)
    cols: int = 4
    cell_width: int = 64
    cell_height: int = 64
    out: str | None = None


@dataclass
class Dependency:
    path: str


@dataclass
class ProjectConfig:
    name: str = "unnamed"
    version: str = "0.1.0"
    entry: str | None = None  # Optional entry point; overrides auto-discovery
    targets: list[RenderTarget] = field(default_factory=list)
    spritesheets: list[SpritesheetTarget] = field(default_factory=list)
    dependencies: dict[str, Dependency] = field(default_factory=dict)
    workspace_members: list[str] = field(default_factory=list)
    project_dir: str = "."


# ======================================================================
# Config loading
# ======================================================================


def load_project_config(path: str | Path) -> ProjectConfig | None:
    p = Path(path)
    if p.is_file():
        config_file = p
        p = p.parent
    else:
        config_file = p / "pml.toml"

    if not config_file.is_file():
        return None

    with config_file.open("rb") as f:
        data = tomllib.load(f)

    config = ProjectConfig(project_dir=str(p.resolve()))

    proj = data.get("project", {})
    config.name = proj.get("name", "unnamed")
    config.version = proj.get("version", "0.1.0")
    config.entry = proj.get("entry")

    render = data.get("render", {})
    raw_targets = render.get("targets", [])
    for t in raw_targets:
        config.targets.append(
            RenderTarget(src=t.get("src", ""), out=t.get("out"), fps=t.get("fps"))
        )

    raw_sheets = render.get("spritesheets", {})
    for name, ss in raw_sheets.items():
        config.spritesheets.append(
            SpritesheetTarget(
                name=name,
                sources=ss.get("sources", []),
                cols=ss.get("cols", 4),
                cell_width=ss.get("cell-width", ss.get("cell_width", 64)),
                cell_height=ss.get("cell-height", ss.get("cell_height", 64)),
                out=ss.get("out"),
            )
        )

    deps = data.get("dependencies", {})
    for name, dep_data in deps.items():
        if isinstance(dep_data, dict) and "path" in dep_data:
            config.dependencies[name] = Dependency(path=dep_data["path"])

    ws = data.get("workspace", {})
    config.workspace_members = ws.get("members", [])

    return config


# ======================================================================
# Target discovery
# ======================================================================


def _find_pml_files(src_dir: Path) -> list[Path]:
    if not src_dir.is_dir():
        return []
    return sorted(src_dir.rglob("*.pml"))


def infer_targets(config: ProjectConfig) -> list[RenderTarget]:
    if config.targets:
        return config.targets
    if config.entry:
        return [RenderTarget(src=config.entry)]
    src_dir = Path(config.project_dir) / "src"
    return [RenderTarget(src=str(f)) for f in _find_pml_files(src_dir)]


def resolve_output_path(target: RenderTarget, config: ProjectConfig) -> str:
    if target.out:
        return str(Path(config.project_dir) / target.out)
    project = Path(config.project_dir)
    src_file = Path(target.src)
    if not src_file.is_absolute():
        src_file = project / target.src
    try:
        rel = src_file.relative_to(project / "src")
    except ValueError:
        rel = Path(src_file.name)
    return str(project / "out" / rel.with_suffix(".png"))


# ======================================================================
# Cache helpers
# ======================================================================


def _file_hash(path: str) -> str:
    h = hashlib.sha256()
    with open(path, "rb") as f:
        h.update(f.read())
    return h.hexdigest()[:16]


_CACHE_FILE = ".pml-cache.json"


def _load_cache(project_dir: str) -> dict[str, str]:
    cf = Path(project_dir) / _CACHE_FILE
    if cf.is_file():
        try:
            return json.loads(cf.read_text())
        except Exception:
            return {}
    return {}


def _save_cache(project_dir: str, cache: dict[str, str]) -> None:
    cf = Path(project_dir) / _CACHE_FILE
    cf.write_text(json.dumps(cache, indent=2), encoding="utf-8")


# ======================================================================
# PMLProject class
# ======================================================================


class PMLProject:
    """Encapsulates a PML project — config, env, module_loader, and all operations."""

    def __init__(self, config: ProjectConfig) -> None:
        self.config = config

    def _make_env(self) -> Any:
        """Create a fresh env with dependency paths injected."""
        env = create_global_env()
        loader = _get_module_loader(env)
        for dep in self.config.dependencies.values():
            dep_path = Path(dep.path)
            if not dep_path.is_absolute():
                dep_path = Path(self.config.project_dir) / dep.path
            if dep_path.is_dir():
                loader.add_search_path(str(dep_path.resolve()))
        return env

    # ── Render single target ─────────────────────────────────────────

    def render_target(
        self, target: RenderTarget, *, quiet: bool = False
    ) -> dict[str, Any]:
        src_path = Path(target.src)
        if not src_path.is_absolute():
            src_path = Path(self.config.project_dir) / target.src
        if not src_path.is_file():
            return {"success": False, "src": target.src, "error": "file not found"}

        out_path = resolve_output_path(target, self.config)
        out_dir = str(Path(out_path).parent)

        saved_dir = _output_dir
        set_output_dir(out_dir)

        env = self._make_env()

        os.makedirs(out_dir, exist_ok=True)
        existing_before = (
            set(Path(out_dir).iterdir()) if Path(out_dir).is_dir() else set()
        )

        try:
            run_file(str(src_path), env=env)
        except Exception as e:
            set_output_dir(saved_dir)
            return {"success": False, "src": target.src, "error": str(e)}

        existing_after = (
            set(Path(out_dir).iterdir()) if Path(out_dir).is_dir() else set()
        )
        new_files = existing_after - existing_before

        if not new_files:
            canvas = get_current_canvas()
            if canvas is not None:
                try:
                    _render(out_path)
                except Exception as e:
                    set_output_dir(saved_dir)
                    return {"success": False, "src": target.src, "error": str(e)}

        set_output_dir(saved_dir)
        result_out = (
            out_path
            if Path(out_path).is_file()
            else (str(list(new_files)[0]) if new_files else out_path)
        )

        if not quiet:
            rel_src = os.path.relpath(src_path, self.config.project_dir)
            rel_out = os.path.relpath(result_out, self.config.project_dir)
            print(f"  render  {rel_src} -> {rel_out}")

        return {"success": True, "src": target.src, "out": result_out}

    # ── Render spritesheet target ────────────────────────────────────

    def render_spritesheet(
        self, sst: SpritesheetTarget, *, quiet: bool = False
    ) -> dict[str, Any]:
        if not quiet:
            print(f"  sheet  {sst.name} ({len(sst.sources)} sprites)")

        project = Path(self.config.project_dir)
        objects: list[Any] = []

        for src in sst.sources:
            env = self._make_env()
            src_path = Path(src)
            if not src_path.is_absolute():
                src_path = project / src
            if not src_path.is_file():
                return {"success": False, "src": sst.name, "error": f"sprite not found: {src}"}
            run_file(str(src_path), env=env)
            canvas = get_current_canvas()
            if canvas and canvas.objects:
                # Flatten the last added object as the sprite
                for obj in canvas.objects:
                    objects.append(obj)
                    break

        if not objects:
            return {"success": False, "src": sst.name, "error": "no sprites collected"}

        out_name = sst.out or f"{sst.name}.png"
        out_path = str(project / "out" / out_name)
        os.makedirs(Path(out_path).parent, exist_ok=True)

        saved_dir = _output_dir
        set_output_dir(str(Path(out_path).parent))

        try:
            _render_spritesheet(
                out_name,
                *objects,
                cols=sst.cols,
                cell_width=sst.cell_width,
                cell_height=sst.cell_height,
            )
        except Exception as e:
            set_output_dir(saved_dir)
            return {"success": False, "src": sst.name, "error": str(e)}

        set_output_dir(saved_dir)
        return {"success": True, "src": sst.name, "out": out_path}

    # ── Render all targets (sequential or parallel) ──────────────────

    def render_all(
        self, *, parallel: bool = False, quiet: bool = False
    ) -> list[dict[str, Any]]:
        results: list[dict[str, Any]] = []

        # Workspace members
        for member in self.config.workspace_members:
            member_path = Path(self.config.project_dir) / member
            member_config = load_project_config(member_path)
            if member_config is None:
                results.append(
                    {"success": False, "workspace_member": member, "error": "no pml.toml"}
                )
                continue
            sub = PMLProject(member_config)
            if not quiet:
                print(f"\n[{member_config.name}]")
            results.extend(sub.render_all(parallel=parallel, quiet=quiet))

        # Targets
        targets = infer_targets(self.config)
        if parallel and len(targets) > 1:
            with concurrent.futures.ThreadPoolExecutor(max_workers=4) as pool:
                futures = [
                    pool.submit(self.render_target, t, quiet=quiet) for t in targets
                ]
                for f in concurrent.futures.as_completed(futures):
                    results.append(f.result())
        else:
            for t in targets:
                results.append(self.render_target(t, quiet=quiet))

        # Spritesheets
        for ss in self.config.spritesheets:
            results.append(self.render_spritesheet(ss, quiet=quiet))

        return results

    # ── Check (validate without rendering) ───────────────────────────

    def check(self) -> list[dict[str, Any]]:
        results: list[dict[str, Any]] = []
        targets = infer_targets(self.config)

        for t in targets:
            src_path = Path(t.src)
            if not src_path.is_absolute():
                src_path = Path(self.config.project_dir) / t.src
            rel_src = os.path.relpath(src_path, self.config.project_dir)

            if not src_path.is_file():
                results.append({"success": False, "src": t.src, "error": "file not found"})
                continue

            try:
                from pml.lexer import Lexer
                from pml.parser import Parser
                from pml.expander import Expander

                source = src_path.read_text(encoding="utf-8")
                tokens = Lexer(source, str(src_path)).tokenize()
                ast = Parser(tokens, str(src_path)).parse()

                env = self._make_env()
                expander = Expander(env)
                expander.expand_all(ast)

                print(f"  check  {rel_src}  OK")
                results.append({"success": True, "src": t.src})
            except Exception as e:
                print(f"  check  {rel_src}  FAIL  {e}")
                results.append({"success": False, "src": t.src, "error": str(e)})

        return results

    # ── Clean ────────────────────────────────────────────────────────

    def clean(self) -> list[str]:
        out_dir = Path(self.config.project_dir) / "out"
        removed: list[str] = []
        if out_dir.is_dir():
            for f in out_dir.rglob("*"):
                if f.is_file():
                    removed.append(str(f))
            shutil.rmtree(out_dir)
            removed.append(str(out_dir))
        cache_file = Path(self.config.project_dir) / _CACHE_FILE
        if cache_file.is_file():
            cache_file.unlink()
            removed.append(str(cache_file))
        return removed

    # ── Watch ────────────────────────────────────────────────────────

    def watch(self) -> None:
        targets = infer_targets(self.config)
        if not targets:
            print("No targets to watch.")
            return

        mtimes: dict[str, float] = {}
        for t in targets:
            src_path = Path(t.src)
            if not src_path.is_absolute():
                src_path = Path(self.config.project_dir) / t.src
            if src_path.is_file():
                mtimes[str(src_path)] = src_path.stat().st_mtime

        print(f"Watching {len(targets)} file(s) for changes... (Ctrl-C to stop)")

        try:
            while True:
                time.sleep(0.5)
                for p_str in list(mtimes):
                    p = Path(p_str)
                    if not p.is_file():
                        continue
                    mtime = p.stat().st_mtime
                    if mtime != mtimes[p_str]:
                        mtimes[p_str] = mtime
                        print(f"\n--- {time.strftime('%H:%M:%S')}  {p.name} changed ---")
                        target = next(
                            (
                                t
                                for t in targets
                                if str(p)
                                == str(Path(self.config.project_dir) / t.src)
                                or str(p) == t.src
                            ),
                            None,
                        )
                        if target:
                            self.render_target(target, quiet=False)
        except KeyboardInterrupt:
            print("\nStopped.")

    # ── Run (render + open) ──────────────────────────────────────────

    def run(self) -> None:
        results = self.render_all()
        opened = 0
        for r in results:
            if r.get("success") and r.get("out"):
                out_path = r["out"]
                if Path(out_path).is_file():
                    _open_file(out_path)
                    opened += 1
        if opened == 0:
            print("No outputs to open.")


# ======================================================================
# Free functions (backward-compatible wrappers)
# ======================================================================


def render_target(target, config, *, quiet=False):
    return PMLProject(config).render_target(target, quiet=quiet)


def render_all(config, *, quiet=False):
    return PMLProject(config).render_all(quiet=quiet)


def watch_render(config):
    return PMLProject(config).watch()


def clean_project(config):
    return PMLProject(config).clean()


def run_project(config):
    return PMLProject(config).run()


# ======================================================================
# Scaffolding (pml new)
# ======================================================================


_NEW_PROJECT_TEMPLATE = """\
[project]
name = "{name}"
version = "0.1.0"
# entry = "src/main.pml"        # Single entry point (use instead of auto-discovery)

[render]
# Auto-discovers src/**/*.pml as render targets (unless entry is set above).
# Uncomment to override:
# targets = [
#   {{ src = "src/main.pml" }},
#   {{ src = "src/animation.pml", fps = 30 }},
# ]

# Spritesheet: combine multiple sprites into one image
# [render.spritesheets]
# "ui-icons" = {{ sources = ["src/icon1.pml", "src/icon2.pml"], cols = 2 }}

# [dependencies]
# "my-lib" = {{ path = "lib/my-lib" }}

# [workspace]
# members = ["packages/foo"]
"""

_MAIN_PML_TEMPLATE = """\
;; {name} — PML project
;; Generated by pml new

(sprite-canvas 128 256 :bg "transparent" :anchor 'center-bottom)

(add (character :expression "happy" :style 'cel))

(render "{name}.png")
"""

_ANIMATION_PML_TEMPLATE = """\
;; {name} — animation example

(canvas 200 200 :bg "#1a1a2e")

(define ball (circle 30 30 20 :fill "#e74c3c"))

(animate ball 'x 30 170 1.0 :ease 'sine-in-out)
(animate ball 'y 30 170 0.4 :ease 'bounce-out)
(play)
(render "{name}_bounce.gif" :fps 30)
"""


def scaffold_project(name: str, dest: str | None = None) -> str:
    project_dir = Path(dest or name).resolve()
    if project_dir.exists():
        print(f"Error: {project_dir} already exists.", file=sys.stderr)
        sys.exit(1)

    src_dir = project_dir / "src"
    src_dir.mkdir(parents=True, exist_ok=True)

    (project_dir / "pml.toml").write_text(
        _NEW_PROJECT_TEMPLATE.format(name=name), encoding="utf-8"
    )
    (src_dir / "main.pml").write_text(
        _MAIN_PML_TEMPLATE.format(name=name), encoding="utf-8"
    )
    (src_dir / "animation.pml").write_text(
        _ANIMATION_PML_TEMPLATE.format(name=name), encoding="utf-8"
    )

    print(f"Created PML project: {project_dir}")
    print(f"  {project_dir / 'pml.toml'}")
    print(f"  {src_dir / 'main.pml'}")
    print(f"  {src_dir / 'animation.pml'}")
    print(f"\nRun:  cd {project_dir.name} && pml render")

    return str(project_dir)


def _open_file(path: str) -> None:
    try:
        if sys.platform == "darwin":
            subprocess.Popen(["open", path])
        elif sys.platform == "win32":
            os.startfile(path)
        else:
            subprocess.Popen(["xdg-open", path])
    except Exception:
        pass