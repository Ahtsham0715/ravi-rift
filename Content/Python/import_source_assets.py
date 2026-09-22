import pathlib
import unreal


PROJECT_ROOT = pathlib.Path(unreal.Paths.project_dir()).resolve()
SOURCE_ART = PROJECT_ROOT / "SourceArt"


def import_files(files, destination):
    tasks = []
    for path in files:
        task = unreal.AssetImportTask()
        task.automated = True
        task.destination_path = destination
        task.filename = str(path)
        task.replace_existing = True
        task.save = True
        tasks.append(task)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)


def main():
    imports = {
        "/Game/RaviCircuit/Concepts": sorted((SOURCE_ART / "Concepts").glob("*.png")),
        "/Game/RaviCircuit/Textures": sorted((SOURCE_ART / "Textures").glob("*.png")),
        "/Game/RaviCircuit/UI/Portraits": sorted((SOURCE_ART / "UI/Portraits").glob("*.png")),
        "/Game/RaviCircuit/UI/MoveIcons": sorted((SOURCE_ART / "UI/MoveIcons").glob("*.png")),
        "/Game/RaviCircuit/UI/Plates": sorted((SOURCE_ART / "UI/Plates").glob("*.png")),
        "/Game/RaviCircuit/VFX": sorted((SOURCE_ART / "VFX").glob("*.png")),
        "/Game/RaviCircuit/Audio": sorted((SOURCE_ART / "Audio").glob("*.wav")),
        "/Game/RaviCircuit/Characters/Blockouts": sorted((SOURCE_ART / "Characters").glob("*.obj")),
        "/Game/RaviCircuit/Stage/Blockouts": sorted((SOURCE_ART / "Stage").glob("*.obj")),
    }
    for destination, files in imports.items():
        if files:
            import_files(files, destination)
            unreal.log(f"Imported {len(files)} files to {destination}")

    unreal.EditorAssetLibrary.save_directory("/Game/RaviCircuit", only_if_is_dirty=False, recursive=True)
    unreal.log("Ravi Rift source asset import complete.")


if __name__ == "__main__":
    main()
