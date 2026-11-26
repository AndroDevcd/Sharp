# Lantern Hunt

A small, complete 3D game written with Sharp's GUI library: a wooded valley at
dusk, eight lit lanterns to find before the light goes.

```
sharpc examples/gui/lantern_hunt/src/main.sharp -o examples/gui/lantern_hunt/lantern_hunt
sharp examples/gui/lantern_hunt/lantern_hunt
```

(The program is built next to `assets/`: it loads its models from the folder it is in.
`sharp examples/gui/lantern_hunt/lantern_hunt play` skips the start screen.)

The corner shows the frames painted each second. The game paints as fast as the
machine can; add `capped` to hold it at 60.

**Playing:** the arrow keys or W A S D walk and turn. Walk into a lantern to
take it. Barrels are in the way.

## What it shows

Everything is in `src/main.sharp`, about 250 lines:

| | |
|---|---|
| land | `terrain(...)` from a height function, with a texture the library draws itself |
| forests | `scatter(pine(3), 900).on(land)` - 900 trees in one line, with a clearing |
| models from files | `model("....gltf")` - the lanterns and the barrels |
| things built from shapes | the explorer: a `group` of a cylinder, a sphere, a cone and cubes |
| light | a low sun with shadows, a `lamp()` in every lantern, fog, an evening sky |
| your own code | `on_frame { ... }` - the keys, walking over the land, bumping into barrels, the following camera |
| widgets over 3D | `hud { ... }` - the count and the clock, the start and end screens |

## The models

`assets/Lantern_01` and `assets/Barrel_01` are from [Poly Haven](https://polyhaven.com)
(CC0 - public domain). Only the files the game uses are kept: the `.gltf`, its `.bin`
and the colour texture.
