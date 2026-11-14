# Text

`<scene/text>` draws text in the scene with a TrueType font from
[eskiu-lang/font](https://github.com/eskiu-lang/font), laid out as ViroReact's `ViroText` is.

```c
Font f = font_load("Inter.ttf")?;
SceneTextStyle st = scene_text_style();
st.size = 24.0;
st.width = 2.0;
st.align = SCENE_TEXT_CENTER;
SceneText label = scene.add_text(0, &f, "Hello, Eskiu", st)?;
scene.node(label.node).position = Vec3{0.0, 1.0, -2.0};
```

`Scene_add_text` adds a node that draws the text in its XY plane, facing +Z, and returns a
`SceneText`: the node, its mesh, material and atlas texture, the realized `width` and
`height` of the laid-out text, and its line count. `SceneText_set` lays out new text or a new
style into the same node, mesh and texture. The font is read while building and is not
kept, so the same `Font` can serve every text.

## Size and box

The font size is in points, and a point is `SCENE_TEXT_POINT` (0.01) world units, so 18
points (the default) gives lines about 0.21 units apart with DejaVu Sans. The text is laid out
in a box `width` x `height` world units (1 x 1 by default), centered on the node.

| Field | Default | Meaning |
|---|---|---|
| `size` | 18 | font size in points |
| `color` | white | linear RGBA, the material's diffuse color |
| `width`, `height` | 1, 1 | the box, in world units |
| `align` | `SCENE_TEXT_LEFT` | `_LEFT`, `_CENTER`, `_RIGHT` in the box |
| `valign` | `SCENE_TEXT_TOP` | `_TOP`, `_MIDDLE`, `_BOTTOM` in the box |
| `line_break` | `SCENE_TEXT_WORD_WRAP` | `_WORD_WRAP`, `_CHAR_WRAP`, `_JUSTIFY`, `_NO_WRAP` |
| `clip` | false | keeps whole lines inside the box (and, with `_NO_WRAP`, cuts each line at the width) |
| `max_lines` | 0 | the most lines drawn (0: no limit) |

## Line breaking

The rules are ViroReact's: characters advance by their glyph's advance (no kerning), a
line breaks after the last word that fits (`_WORD_WRAP`, a word being its characters and
the spaces after it), at any character (`_CHAR_WRAP`), or only at newlines
(`_NO_WRAP`). A word wider than the box gets a line of its own. A newline always starts a
new line. `_JUSTIFY` wraps each paragraph by words and stretches the spaces of every line
but the paragraph's last to the box width; ViroReact picks the breaks with Knuth and
Plass's algorithm, so a justified paragraph may break at other words than here.

Vertical alignment places the first baseline so the line box (the font's line height,
from its descent below the baseline) sits at the top of the box, around its middle, or so
the last line ends at its bottom. With `clip`, a line that would cross the bottom of the
box is not drawn; `max_lines` stops before the limit whatever the box.

`scene_text_size(f, text, style, &w, &h)` returns the laid-out size without building
anything (ViroReact's `getTextSize`).

## Drawing

The glyphs of the characters in the text are rendered into one atlas at four times the
font size, a texture whose alpha is the glyph coverage, and each character is a quad
sampling it. The material is unlit (`SCENE_CONSTANT`) and blended: change it through
`Scene_material(label.material)`, for a lit text, another color, or `skip_depth_test` with
a `rendering_order` to keep a label on top. The text is single-sided: seen from behind it
is not drawn unless the material is `double_sided`.
