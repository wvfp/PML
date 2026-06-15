# PML Phase 4 实现计划 — 扩展语义组件

## 目标

在 Phase 2+3 基础上，实现 12 个扩展语义组件：服装(outfit)、道具(weapon/potion/chest/generic-item)、UI 控件(button/panel/health-bar/icon)、场景元素(tile/decoration/background)。

**最终验证**：能生成完整的角色+装备 sprite、UI 控件组、场景瓦片。

---

## 实现步骤

### Step 1：Outfit 组件 (`pml/sprites/components/outfit.py`)

服装组件，覆盖角色身体区域。

```
(outfit
  :top 'hoodie | 't-shirt | 'jacket | 'dress | 'armor | 'robe | 'suit | 'tank | 'sailor
  :bottom 'pants | 'skirt | 'shorts | 'long-skirt | 'armor
  :shoes 'boots | 'sneakers | 'sandals | 'heels | 'none
  :color-top <color>
  :color-bottom <color>
  :detail 'plain | 'striped | 'pattern | 'badge)
```

绘制逻辑：上装覆盖 torso 区域，下装覆盖 torso 下半部，鞋子在底部。使用 rect + ellipse + line 组合。

### Step 2：道具组件 (`pml/sprites/components/items.py`)

4 个道具组件合并在一个文件中：

**weapon**:
```
(weapon
  :type 'sword | 'axe | 'bow | 'staff | 'dagger | 'spear | 'gun
  :size 'small | 'medium | 'large | 'legendary
  :material 'iron | 'steel | 'gold | 'crystal | 'wood | 'bone
  :element 'none | 'fire | 'ice | 'lightning | 'holy | 'dark
  :ornament 'plain | 'gem | 'rune | 'engraved)
```

**potion**:
```
(potion
  :type 'health | 'mana | 'buff | 'poison | 'bomb
  :container 'bottle | 'flask | 'vial | 'jar
  :color <color>
  :size 'small | 'medium | 'large)
```

**chest**:
```
(chest
  :type 'wooden | 'iron | 'gold | 'crystal
  :state 'closed | 'open | 'locked
  :size 'small | 'medium | 'large)
```

**generic-item**:
```
(generic-item
  :name <string>
  :base-shape 'circle | 'rect | 'diamond | 'custom
  :color <color>
  :detail <graphic-object>
  :outline <boolean>)
```

### Step 3：UI 控件 (`pml/sprites/components/ui_widgets.py`)

**button**:
```
(button
  :label <string>
  :width <number>
  :height <number>
  :style 'rounded | 'sharp | 'pixel | 'ornate
  :state 'normal | 'hover | 'pressed | 'disabled
  :color <color>
  :text-color <color>)
```

**panel**:
```
(panel
  :width <number>
  :height <number>
  :style 'simple | 'ornate | 'pixel | 'glass
  :title <string>
  :color <color>
  :border-width <number>)
```

**health-bar**:
```
(health-bar
  :value <number 0~1>
  :width <number>
  :height <number>
  :color <color>
  :bg-color <color>
  :style 'flat | 'segmented | 'gradient)
```

**icon**:
```
(icon
  :type 'heart | 'star | 'coin | 'gem | 'shield | 'skull
  :size <number>
  :color <color>
  :style 'flat | 'pixel | 'detailed)
```

### Step 4：场景元素 (`pml/sprites/components/scene_elements.py`)

**tile**:
```
(tile
  :type 'grass | 'stone | 'wood | 'sand | 'water | 'snow | 'brick
  :size <number>          ; default 32
  :variant <number 0~3>
  :edge 'none | 'top | 'bottom | 'left | 'right)
```

**decoration**:
```
(decoration
  :type 'tree | 'bush | 'rock | 'flower | 'mushroom | 'crate | 'barrel | 'torch | 'sign | 'fence | 'lamp
  :size 'small | 'medium | 'large
  :season 'spring | 'summer | 'autumn | 'winter
  :variant <number>)
```

**background**:
```
(background
  :type 'sky | 'forest | 'dungeon | 'town | 'ocean | 'mountain
  :time 'day | 'dusk | 'night | 'dawn
  :weather 'clear | 'cloudy | 'rain | 'snow | 'fog
  :width <number>
  :height <number>
  :parallax <number>)     ; default 1.0
```

### Step 5：更新 Character 组装器

更新 `character.py` 支持 `:outfit` 和 `:weapon` 参数：
- `:outfit <outfit-obj>` — 覆盖在 body 上
- `:weapon <weapon-obj>` — 放置在角色侧面

### Step 6：注册新组件

更新 `registry.py` 注册 outfit/weapon/potion/chest/generic-item/button/panel/health-bar/icon/tile/decoration/background。

### Step 7：测试 + 示例

- 单元测试：每个组件的构造、参数验证、fallback 行为
- 集成测试：角色+装备渲染、UI 控件组渲染、场景渲染
- 示例 `examples/phase4.pml`：完整演示

---

## 新增/修改文件

```
pml/sprites/components/
├── outfit.py           (NEW) — 服装组件
├── items.py            (NEW) — weapon + potion + chest + generic-item
├── ui_widgets.py       (NEW) — button + panel + health-bar + icon
├── scene_elements.py   (NEW) — tile + decoration + background
├── character.py        (MODIFY) — 支持 :outfit 和 :weapon

pml/sprites/
├── registry.py         (MODIFY) — 注册 12 个新组件

tests/
├── test_phase4.py      (NEW) — Phase 4 测试

examples/
├── phase4.pml          (NEW) — Phase 4 示例
```

**预估**：新增约 1200-1500 行 Python
