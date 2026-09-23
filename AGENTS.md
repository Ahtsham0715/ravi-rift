# Agent Handoff Log

## Project Direction

- User explicitly changed direction to Unreal only: do not use Godot.
- 3D work should use Unreal or Unity, with Unreal preferred/required for this project.
- Build target is macOS.
- Public-facing game name is **Ravi Rift**. Keep the underlying Unreal module/project filenames as `RaviCircuit` unless doing a deliberate rename pass after the first successful Unreal build.
- Quality target: Tekken-rival class 3D arena fighter depth, responsiveness and presentation, but with fully original IP, characters, stages, moves, UI, audio and assets. Do not use copyrighted Tekken assets, names, likenesses, stages, animations or UI.

## Environment Findings

- Repository started empty.
- Godot 4.7.2 is installed but was rejected by the user. Temporary Godot files were removed.
- Unreal Engine and Epic Games Launcher are not installed in `/Applications`, `/Users/Shared/Epic Games`, or nearby user paths.
- Update after user action: Epic Games Launcher is now installed at `/Applications/Epic Games Launcher.app`.
- Unreal Engine 5.8 is now installed at `/Users/Shared/Epic Games/UE_5.8`; `GenerateProjectFiles.sh`, `Build.sh`, and `UnrealEditor.app` are present.
- `./Scripts/build_unreal_mac.sh` was fixed to run Epic's Mac scripts from `Engine/Build/BatchFiles/Mac`, then successfully generated project files and built `RaviCircuitEditor`.
- Disk filled during the UE 5.8 download; disposable user caches were cleared. After build/install cleanup there was roughly 17 GB free. Continue watching disk space.
- Homebrew does not expose an Unreal/Epic cask here.
- Xcode is installed: Xcode 26.4 build 17E192.
- Epic's current UE 5.8 macOS requirements list Xcode 26.4 as incompatible, with Xcode 26.1.1 recommended, but the current editor target did compile with Xcode 26.4.
- `git ls-remote https://github.com/EpicGames/UnrealEngine.git` failed with repository not found, so the current GitHub auth is not linked/authorized for Epic's private Unreal source repository.
- Official install route requires Epic Games Launcher, Epic sign-in, and EULA acceptance before Unreal can be downloaded. Launcher has been opened for the user.

## Current Repo State

- Public GitHub repository: `https://github.com/Ahtsham0715/ravi-rift`
- Initial commit pushed: `b7611cf` (`Initial Ravi Rift Unreal fighting game slice`)
- Unreal C++ project scaffold created:
  - `RaviCircuit.uproject`
  - `Config/DefaultEngine.ini`
  - `Config/DefaultGame.ini`
  - `Config/DefaultInput.ini`
  - `Source/RaviCircuit.Target.cs`
  - `Source/RaviCircuitEditor.Target.cs`
  - `Source/RaviCircuit/RaviCircuit.Build.cs`
  - `Source/RaviCircuit/RaviCircuit.cpp`
  - `Source/RaviCircuit/RCMoveData.h`
  - `Source/RaviCircuit/RCFighterCharacter.h`
  - `Build/Mac/Resources/PrivacyInfo.xcprivacy`
  - `Tools/generate_source_assets.py`
  - `Content/Python/import_source_assets.py`
  - `Docs/AssetPipeline.md`
  - `SourceArt/`
  - `Scripts/build_unreal_mac.sh`
- `README.md` documents the install/build blocker and controls.
- Build helper is restored and executable. On this filesystem the path may display as `scripts/build_unreal_mac.sh`, but `./Scripts/build_unreal_mac.sh` works.
- Last successful local build: `./Scripts/build_unreal_mac.sh` completed with `Result: Succeeded` and output binary `/Users/Shared/Epic Games/UE_5.8/Engine/Binaries/Mac/UnrealEditor`.

## Implemented So Far

- Data-driven move library in `RCMoveData.h`.
- Two original fighters:
  - Zara Vey: faster magnetic kickboxing.
  - Hamza Kade: heavier pressure grappler.
- Move properties include startup, active, recovery, hit level, damage, chip, hit/block advantage, launch, range, pushback, tracking, meter, super flag, cancels.
- Fighter header declares combat state machine, local/CPU control hooks, wall interaction, combo/rage/meter/training history fields.
- `RCFighterCharacter.cpp` implemented:
  - Runtime mesh fighter assembly from Unreal basic meshes.
  - P1 input and P2 keyboard input.
  - CPU spacing/guard/attack selection.
  - 60 Hz frame accumulator for move/state frames.
  - Blocking, crouch/high/low/throw rules, launch, juggle scaling, wall splat checks, knockdown and rage/meter behavior.
  - Procedural pose animation for idle, attacks, hit stun and knockdown.
- `RaviCircuitGameMode`, `RCFightCameraActor` and `RCHUD` implemented:
  - Code-generated nighttime urban rooftop arena with walls, rails, skyline, neon signs, fog and lights.
  - Best-of-3 match flow, timer, pause, hit announcements and training line.
  - Dynamic fight camera with cinematic super timing and hit shake.
  - Canvas HUD for health, meter, timer, rounds, names, controls and pause.
- Front-end and mode flow added:
  - Title/menu is rendered in C++ HUD.
  - Front-end now owns a camera view of the generated arena before fighters spawn.
  - `1` starts Arcade / VS CPU.
  - `2` starts Local Versus.
  - `3` starts Training.
  - `4` hosts an online listen-server match.
  - `5` joins `127.0.0.1` for local online-client testing.
  - `Q` / `E` swap P1/P2 fighters before match start.
  - Post-match screen supports `R` rematch and `M`/Esc main menu.
- Single-player and multiplayer support:
  - Arcade / VS CPU and Training cover single-player.
  - Local Versus creates/possesses a second local player controller for same-machine multiplayer.
  - Online Host/Join is scaffolded through Unreal listen-server travel.
  - `ARCFighterCharacter` replicates fighter state and sends client move/input data to the server with RPCs.
  - `ARaviCircuitGameMode::PostLogin` assigns a remote challenger to P2 and sets the fight camera.
  - This is not final rollback netcode yet; see `Docs/MultiplayerPlan.md`.
- Training mode improved:
  - Training dummy no longer runs CPU behavior by default.
  - `F` resets positions/health.
  - `G` toggles dummy guard.
  - `H` toggles stand/low guard.
  - HUD shows P1 move list and frame data.
- Combat feel additions:
  - 8-frame input buffer for attacks during recovery/stun.
  - Counter-hit detection when interrupting opponent startup, with damage multiplier and HUD announcement.
  - Back-to-block support in addition to explicit block button, without overwriting CPU/training forced guard.
  - Wake-up options from knockdown:
    - Sidestep input late in knockdown performs a wake roll.
    - Kick/low/special late in knockdown performs a wake-up attack.
- Settings/comfort toggles:
  - `V` toggles hit VFX.
  - `C` toggles camera shake.
  - `B` toggles haptics.
  - Menu/HUD text reflects these toggles.
- Source assets generated:
  - `SourceArt/Audio/*.wav`: original synthesized PCM placeholder sounds for hits, blocks, UI, round start, KO, wall splat, counter hit, throw hit, rage ready and super riser.
  - `SourceArt/Concepts/*.png`: generated stage and fighter concept/key-art plates.
  - `SourceArt/Data/move_list.json`: move list/frame-data source reference.
  - `SourceArt/Characters/*.obj`: Zara/Hamza blockout meshes with `.mtl` materials.
  - `SourceArt/Stage/*.obj`: rooftop boundary and neon prop blockouts.
  - `SourceArt/Textures/*.png`: generated material/trim textures.
  - `SourceArt/UI/**/*.png`: generated portraits, move icons, plates and Ravi Rift logo plate.
  - `SourceArt/VFX/impact_vfx_spritesheet.png`: generated transparent-background impact VFX sprite sheet.
  - `SourceArt/Concepts/character_select_background.png`: generated modern character-select background with no baked text.
  - `SourceArt/Scenes/noorabad_rooftop_layout.json`: scene/source-art manifest.
  - Regenerate base assets via `python3 Tools/generate_source_assets.py`.
  - Regenerate derived UI/texture/data assets via `python3 Tools/generate_derived_assets.py`.
  - Import into Unreal via `Content/Python/import_source_assets.py` once the editor is available.
  - Attempted unattended import with `UnrealEditor-Cmd -run=pythonscript`; it hung without logs or `.uasset` output, so use the editor Python console for import unless the commandlet path is debugged.
- Audio integration:
  - `RaviCircuitGameMode` loads imported `/Game/RaviCircuit/Audio/*` assets if present.
  - Menu ticks/confirm/back, hits, blocks, wall splats, KO, throws, counter hits, round starts, announcer-style fight/KO/perfect cues and super risers play imported sounds when available and safely skip when not imported.
  - `Docs/AudioDirection.md` documents the original audio/announcer direction and warns against copying existing fighting-game voices or samples.
- Controller/haptics:
  - P1 supports keyboard and gamepad.
  - Left stick or D-pad movement, face-button attacks, shoulder/trigger special/super/block and Start pause.
  - Dynamic force feedback varies by block, light/heavy hit, counter hit, wall splat, round start and super where supported.
- Concept art integration:
  - `RCHUD` optionally loads imported `/Game/RaviCircuit/Concepts/*` textures.
  - Title and match-complete screens draw the stage key art and fighter concept plates if imported; otherwise they fall back to text-only HUD.
- `Docs/AssetPipeline.md` describes import steps and replacement targets for real skeletal/static mesh assets after Unreal Editor is available.
- Static cleanup performed:
  - Engine entry map is used instead of a missing project map.
  - Fighter ticks respect match pause.
  - Hit spark lifespan is applied to the owner actor.
  - GameMode no longer auto-spawns a default pawn before the explicit P1/P2 match spawn.
  - `Scripts/build_unreal_mac.sh` is executable.
  - Multi-line HUD text renderer added for menu/training text.
  - Safer `FindChecked` fighter lookup and explicit `FString` concatenation.
  - Added macOS privacy manifest for modern Xcode packaging expectations.
  - Build helper now distinguishes a missing Unreal install from a partial Epic Launcher install.
  - Generated meshes now prefer `/Engine/BasicShapes/BasicShapeMaterial` and set both `Color` and `BaseColor` params so runtime colors are more likely to appear correctly.
  - Target files moved to `BuildSettingsVersion.V7` for UE 5.8.
  - UE 5.8 compile fixes added: no-arg input action wrappers, concrete fog component include and point-light component attenuation call.
  - Mac packaging resources generated/kept under `Build/Mac/Resources`, including network server entitlements useful for online hosting.

## Next Steps

- Full compile currently passes with Unreal 5.8 via `./Scripts/build_unreal_mac.sh`.
- Multiplayer compile/QA still needs Unreal: test host via menu key `4`, then run a second client joining `127.0.0.1` via menu key `5`.
- Monitor Xcode 26.4 closely for packaging/runtime issues because Epic's macOS requirements still recommend Xcode 26.1.1 for UE 5.8, even though the current editor compile succeeds.
- Import generated assets from an open Unreal Editor session using `Content/Python/import_source_assets.py`; commandlet import needs investigation.
- Optional later: replace the code-rendered Canvas HUD/menu with a Blueprint or UMG front-end once Unreal Editor is available.
- Replace procedural placeholder meshes with proper skeletal meshes/animations when asset pipeline is available.
- Keep this file updated after each major subsystem lands.
