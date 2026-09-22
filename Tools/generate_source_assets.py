#!/usr/bin/env python3
import json
import math
import struct
import wave
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_ART = ROOT / "SourceArt"


def ensure_dirs():
    for sub in ["Audio", "Characters", "Stage", "Scenes"]:
        (SOURCE_ART / sub).mkdir(parents=True, exist_ok=True)


def write_wav(path: Path, frequency: float, duration: float, volume: float, sweep: float = 0.0, noise: float = 0.0):
    rate = 44100
    frames = int(rate * duration)
    with wave.open(str(path), "w") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(rate)
        data = bytearray()
        seed = 1337
        for i in range(frames):
            t = i / rate
            env = max(0.0, 1.0 - (i / max(1, frames - 1)))
            freq = frequency + sweep * t
            tone = math.sin(2.0 * math.pi * freq * t)
            seed = (1103515245 * seed + 12345) & 0x7FFFFFFF
            n = ((seed / 0x7FFFFFFF) * 2.0 - 1.0) * noise
            sample = (tone * (1.0 - noise) + n) * volume * env * env
            data.extend(struct.pack("<h", int(max(-1.0, min(1.0, sample)) * 32767)))
        f.writeframes(bytes(data))


def cube_obj(name, center, scale, material):
    cx, cy, cz = center
    sx, sy, sz = scale
    verts = [
        (cx - sx, cy - sy, cz - sz), (cx + sx, cy - sy, cz - sz), (cx + sx, cy + sy, cz - sz), (cx - sx, cy + sy, cz - sz),
        (cx - sx, cy - sy, cz + sz), (cx + sx, cy - sy, cz + sz), (cx + sx, cy + sy, cz + sz), (cx - sx, cy + sy, cz + sz),
    ]
    faces = [(1, 2, 3, 4), (5, 8, 7, 6), (1, 5, 6, 2), (2, 6, 7, 3), (3, 7, 8, 4), (4, 8, 5, 1)]
    return {"name": name, "verts": verts, "faces": faces, "material": material}


def write_obj(path: Path, objects, materials):
    mtl_path = path.with_suffix(".mtl")
    with mtl_path.open("w", encoding="utf-8") as mtl:
        for name, color in materials.items():
            mtl.write(f"newmtl {name}\nKd {color[0]} {color[1]} {color[2]}\nKs 0.08 0.08 0.08\nNs 24\n\n")
    with path.open("w", encoding="utf-8") as obj:
        obj.write(f"mtllib {mtl_path.name}\n")
        offset = 0
        for item in objects:
            obj.write(f"o {item['name']}\nusemtl {item['material']}\n")
            for v in item["verts"]:
                obj.write(f"v {v[0]:.4f} {v[1]:.4f} {v[2]:.4f}\n")
            for face in item["faces"]:
                obj.write("f " + " ".join(str(idx + offset) for idx in face) + "\n")
            offset += len(item["verts"])


def character_blockout(name, bulky=False):
    width = 0.34 if not bulky else 0.42
    height = 1.0 if not bulky else 0.96
    objects = [
        cube_obj("torso", (0, 0, 1.22 * height), (width, 0.22, 0.42), "primary"),
        cube_obj("head", (0, -0.02, 2.02 * height), (0.21, 0.19, 0.23), "skin"),
        cube_obj("left_arm", (-0.45 * width / 0.34, 0, 1.25 * height), (0.09, 0.11, 0.44), "accent"),
        cube_obj("right_arm", (0.45 * width / 0.34, 0, 1.25 * height), (0.09, 0.11, 0.44), "accent"),
        cube_obj("left_leg", (-0.18, 0, 0.48 * height), (0.11, 0.12, 0.48), "dark"),
        cube_obj("right_leg", (0.18, 0, 0.48 * height), (0.11, 0.12, 0.48), "dark"),
        cube_obj("sash", (0, -0.24, 1.04 * height), (0.48, 0.035, 0.055), "accent"),
    ]
    colors = {
        "primary": (0.05, 0.78, 0.96) if not bulky else (0.95, 0.22, 0.14),
        "accent": (1.0, 0.77, 0.12) if not bulky else (0.28, 0.95, 0.48),
        "dark": (0.025, 0.025, 0.05) if not bulky else (0.055, 0.03, 0.02),
        "skin": (0.66, 0.42, 0.31) if not bulky else (0.72, 0.46, 0.32),
    }
    write_obj(SOURCE_ART / "Characters" / f"{name}_blockout.obj", objects, colors)


def stage_assets():
    mats = {
        "floor": (0.05, 0.055, 0.068),
        "neon_cyan": (0.05, 0.9, 1.0),
        "neon_gold": (1.0, 0.74, 0.08),
        "wall": (0.95, 0.32, 0.08),
        "dark": (0.06, 0.065, 0.08),
    }
    boundary = [
        cube_obj("floor", (0, 0, -0.08), (8.0, 4.0, 0.09), "floor"),
        cube_obj("left_wall", (-7.95, 0, 1.3), (0.08, 4.0, 1.3), "wall"),
        cube_obj("right_wall", (7.95, 0, 1.3), (0.08, 4.0, 1.3), "wall"),
        cube_obj("back_rail", (0, -4.1, 0.62), (8.0, 0.07, 0.55), "dark"),
        cube_obj("front_rail", (0, 4.1, 0.62), (8.0, 0.07, 0.55), "dark"),
    ]
    write_obj(SOURCE_ART / "Stage" / "noorabad_rooftop_boundary.obj", boundary, mats)
    signs = [
        cube_obj("cyan_sign", (-1.8, 0, 1.2), (0.75, 0.04, 0.16), "neon_cyan"),
        cube_obj("gold_sign", (1.8, 0, 1.35), (0.75, 0.04, 0.16), "neon_gold"),
        cube_obj("arch_left", (-2.8, 0, 1.7), (0.08, 0.08, 1.0), "neon_gold"),
        cube_obj("arch_top", (0, 0, 2.7), (2.9, 0.08, 0.08), "neon_gold"),
        cube_obj("arch_right", (2.8, 0, 1.7), (0.08, 0.08, 1.0), "neon_gold"),
    ]
    write_obj(SOURCE_ART / "Stage" / "neon_arch_and_signs.obj", signs, mats)


def manifests():
    scene = {
        "name": "Noorabad Rooftop Arena",
        "mood": "nighttime urban Punjab/Pakistan-inspired rooftop arena",
        "bounds": {"x": [-720, 720], "y": [-315, 315]},
        "fighters": ["Zara Vey", "Hamza Kade"],
        "source_art": {
            "boundary": "SourceArt/Stage/noorabad_rooftop_boundary.obj",
            "neon": "SourceArt/Stage/neon_arch_and_signs.obj",
            "zara": "SourceArt/Characters/zara_vey_blockout.obj",
            "hamza": "SourceArt/Characters/hamza_kade_blockout.obj"
        },
        "audio": [
            "SourceArt/Audio/impact_light.wav",
            "SourceArt/Audio/impact_heavy.wav",
            "SourceArt/Audio/block_guard.wav",
            "SourceArt/Audio/super_riser.wav",
            "SourceArt/Audio/menu_tick.wav"
        ],
        "import_notes": "These are original placeholder source assets. Import into Unreal once the editor is available, or keep using the runtime-generated C++ scene."
    }
    (SOURCE_ART / "Scenes" / "noorabad_rooftop_layout.json").write_text(json.dumps(scene, indent=2) + "\n", encoding="utf-8")


def main():
    ensure_dirs()
    write_wav(SOURCE_ART / "Audio" / "impact_light.wav", 180.0, 0.16, 0.5, sweep=-80.0, noise=0.35)
    write_wav(SOURCE_ART / "Audio" / "impact_heavy.wav", 96.0, 0.28, 0.75, sweep=-42.0, noise=0.42)
    write_wav(SOURCE_ART / "Audio" / "block_guard.wav", 310.0, 0.12, 0.38, sweep=120.0, noise=0.18)
    write_wav(SOURCE_ART / "Audio" / "super_riser.wav", 110.0, 0.9, 0.35, sweep=680.0, noise=0.08)
    write_wav(SOURCE_ART / "Audio" / "menu_tick.wav", 620.0, 0.07, 0.25, sweep=180.0, noise=0.02)
    write_wav(SOURCE_ART / "Audio" / "menu_confirm.wav", 760.0, 0.12, 0.32, sweep=260.0, noise=0.01)
    write_wav(SOURCE_ART / "Audio" / "menu_back.wav", 420.0, 0.10, 0.24, sweep=-180.0, noise=0.01)
    write_wav(SOURCE_ART / "Audio" / "round_start.wav", 210.0, 0.62, 0.42, sweep=260.0, noise=0.08)
    write_wav(SOURCE_ART / "Audio" / "ko_hit.wav", 72.0, 0.72, 0.88, sweep=-35.0, noise=0.5)
    write_wav(SOURCE_ART / "Audio" / "wall_splat.wav", 82.0, 0.36, 0.78, sweep=-70.0, noise=0.55)
    write_wav(SOURCE_ART / "Audio" / "counter_hit.wav", 510.0, 0.24, 0.58, sweep=420.0, noise=0.18)
    write_wav(SOURCE_ART / "Audio" / "throw_hit.wav", 118.0, 0.34, 0.72, sweep=-22.0, noise=0.38)
    write_wav(SOURCE_ART / "Audio" / "rage_ready.wav", 150.0, 1.05, 0.38, sweep=520.0, noise=0.06)
    character_blockout("zara_vey", bulky=False)
    character_blockout("hamza_kade", bulky=True)
    stage_assets()
    manifests()


if __name__ == "__main__":
    main()
