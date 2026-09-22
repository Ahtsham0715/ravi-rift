# Source Art

Original placeholder source assets for Ravi Rift.

## Contents

- `Audio/*.wav`: synthesized PCM source sounds for impacts, block, UI, round start, KO, wall splat, counter hit, throw hit, rage ready and super riser.
- `Concepts/*.png`: generated concept/key-art plates for stage and fighters.
- `Data/move_list.json`: move-list/frame-data reference for UI and docs.
- `Characters/*.obj`: simple original blockout meshes for Zara Vey and Hamza Kade.
- `Stage/*.obj`: rooftop boundary and neon prop blockouts for Noorabad Rooftop Arena.
- `Textures/*.png`: generated material and trim texture sources.
- `UI/**/*.png`: generated portraits, move icons, plates and logo source art.
- `VFX/impact_vfx_spritesheet.png`: generated transparent-background impact VFX sprite sheet.
- `Scenes/noorabad_rooftop_layout.json`: scene manifest tying the source art to the code-generated arena.

Regenerate these files with:

```sh
python3 Tools/generate_source_assets.py
python3 Tools/generate_derived_assets.py
```

These are source assets, not cooked `.uasset` files. Import them in Unreal Editor once the engine install finishes, or continue using the C++ runtime-generated scene.

## Generated Image Prompts

The concept plates were generated with the built-in image generation tool and saved into this workspace:

- `Concepts/noorabad_rooftop_key_art.png`: nighttime urban rooftop fighting arena inspired by modern Punjab/Pakistan architecture, with cyan/amber neon, wet concrete, skyline, spectators and readable arena boundaries.
- `Concepts/character_select_background.png`: modern rainy rooftop character-select background with implied portrait slots and no baked text.
- `Concepts/zara_vey_concept.png`: Zara Vey, agile magnetic kickboxer, cyan/gold/charcoal tactical materials, original full-body character concept.
- `Concepts/hamza_kade_concept.png`: Hamza Kade, heavy pressure grappler, red/green/charcoal reinforced outfit with geometric embroidery accents, original full-body character concept.
- `Concepts/versus_splash.png`: wide rainy rooftop versus composition with center negative space for code-rendered UI.
