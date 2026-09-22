# Ravi Rift

Original Unreal Engine 5 3D fighting game vertical slice for macOS.

Quality target: a rival-grade modern 3D arena fighter with serious frame-data combat and cinematic presentation, using fully original IP and assets.

## Current Build State

This repo contains a C++ Unreal project scaffold with runtime-generated arena, two original fighters, data-driven frame moves, combat state machine, AI, local keyboard P2 controls, title/menu flow, HUD, camera, hit VFX and training instrumentation.

Original placeholder source assets live in `SourceArt/`:

- Synthesized WAVs for impacts, block, super riser and menu tick.
- Additional WAVs for menu confirm/back, round start, KO, wall splat, counter hit, throw hit and rage ready.
- Generated PNG concept plates for the stage and both fighters.
- Generated PNG UI plates, portraits, move icons, VFX sprite sheet and material textures.
- OBJ blockouts for Zara Vey, Hamza Kade and the Noorabad Rooftop Arena props.
- A scene manifest at `SourceArt/Scenes/noorabad_rooftop_layout.json`.

Regenerate them with:

```sh
python3 Tools/generate_source_assets.py
python3 Tools/generate_derived_assets.py
```

After Unreal Editor is available, import the source assets from the editor Python console or command line with:

```sh
py Content/Python/import_source_assets.py
```

Unreal Engine is not installed on this machine. Epic's official installer requires Epic Games Launcher sign-in and EULA acceptance before the engine download begins. Epic's current macOS requirements also list Xcode 26.4 as incompatible with UE 5.8, while this machine currently has Xcode 26.4.

## Build Once Unreal Is Installed

Install a compatible Unreal Engine version through Epic Games Launcher, then run:

```sh
./Scripts/build_unreal_mac.sh
```

The script searches common macOS Unreal install locations, generates Xcode project files, and builds the editor target.

## Controls

Menu: `1` Arcade / VS CPU, `2` Local Versus, `3` Training, `Q` swap P1, `E` swap P2.

P1: WASD movement/crouch/jump, Q/E sidestep, J punch, K kick, L low, U throw, I special, O super, Shift block.

Controller: left stick or D-pad movement, face buttons attacks, shoulders/triggers special/super/block, Start pause. Runtime haptics are enabled for hits, blocks, wall splats, round start and supers where supported.

P2 local: Arrow keys movement, comma/period sidestep, numpad 1/2/3/4/5/6 attacks, right Ctrl block.

Training: `F` reset, `G` toggle dummy guard, `H` toggle stand/low guard.

Post-match: `R` rematch, `M` or Esc main menu. Settings hotkeys: `V` hit VFX, `C` camera shake, `B` haptics.
