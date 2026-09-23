# Asset Pipeline

## Current Runtime Assets

The game currently runs from C++ generated primitives and optional imported source art.

- Fighters: procedural mesh parts in `ARCFighterCharacter::BuildVisuals`.
- Stage: procedural blocks/lights/fog in `ARaviCircuitGameMode::BuildArena`.
- HUD: Canvas HUD in `ARCHUD`.
- Optional imported art: `SourceArt/Concepts`, `SourceArt/Audio`, `SourceArt/Characters`, `SourceArt/Stage`.

## Import Steps After Unreal Installs

1. Run `python3 Tools/generate_source_assets.py` if source assets need regeneration.
2. Open the project in Unreal Editor.
3. Run `Content/Python/import_source_assets.py` from the editor Python console. The attempted unattended `UnrealEditor-Cmd -run=pythonscript` path hung on this machine without logs, so prefer an open editor session unless that is fixed.
4. Verify imported assets under `/Game/RaviCircuit`.

## Replacement Targets

- Replace `BuildVisuals` mesh parts with skeletal meshes for Zara and Hamza.
- Convert move startup/active/recovery into animation notifies or montage section events.
- Replace blockout stage meshes with authored static meshes.
- Convert Canvas HUD to UMG when polish pass begins.
- Keep `RCMoveData.h` as the gameplay source of truth until a DataAsset migration is useful.

## Art Direction Anchors

- Zara Vey: agile magnetic kickboxer, cyan/gold/charcoal technical gear, fast silhouette.
- Hamza Kade: heavy pressure grappler, red/green/charcoal reinforced fabric, broad silhouette.
- Noorabad Rooftop: nighttime rooftop arena, wet concrete, cyan/amber neon, arches, patterned metalwork, city haze.
