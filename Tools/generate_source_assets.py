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


def _rng(seed: int):
    value = seed & 0x7FFFFFFF
    while True:
        value = (1103515245 * value + 12345) & 0x7FFFFFFF
        yield (value / 0x7FFFFFFF) * 2.0 - 1.0


def write_layered_wav(path: Path, duration: float, layers, seed: int = 9001):
    rate = 44100
    frames = int(rate * duration)
    noise_iter = _rng(seed)
    samples = []
    for i in range(frames):
        t = i / rate
        sample = 0.0
        n = next(noise_iter)
        for layer in layers:
            start = layer.get("start", 0.0)
            if t < start:
                continue
            lt = t - start
            length = layer.get("duration", duration - start)
            if lt > length:
                continue
            p = lt / max(0.0001, length)
            attack = layer.get("attack", 0.004)
            release_shape = layer.get("release", 2.0)
            env = min(1.0, lt / max(attack, 0.0001)) * ((1.0 - p) ** release_shape)
            amp = layer.get("amp", 0.4) * env
            kind = layer.get("kind", "sine")
            freq = layer.get("freq", 120.0) + layer.get("sweep", 0.0) * p
            if kind == "noise":
                value = n
            elif kind == "square":
                value = 1.0 if math.sin(2.0 * math.pi * freq * lt) >= 0 else -1.0
            elif kind == "saw":
                value = 2.0 * ((freq * lt) % 1.0) - 1.0
            elif kind == "formant":
                value = 0.0
                for mult, weight in layer.get("partials", [(1.0, 1.0), (2.1, 0.45), (3.8, 0.22)]):
                    value += math.sin(2.0 * math.pi * freq * mult * lt) * weight
            else:
                value = math.sin(2.0 * math.pi * freq * lt)
            sample += value * amp
        samples.append(sample)
    peak = max(0.001, max(abs(s) for s in samples))
    scale = 0.92 / peak
    with wave.open(str(path), "w") as f:
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(rate)
        data = bytearray()
        for sample in samples:
            data.extend(struct.pack("<h", int(max(-1.0, min(1.0, sample * scale)) * 32767)))
        f.writeframes(bytes(data))


def write_impact(path: Path, heavy: bool, seed: int):
    write_layered_wav(path, 0.22 if not heavy else 0.42, [
        {"kind": "noise", "amp": 0.75 if heavy else 0.48, "duration": 0.055 if heavy else 0.032, "release": 4.5},
        {"kind": "sine", "freq": 72 if heavy else 160, "sweep": -36 if heavy else -80, "amp": 0.8 if heavy else 0.38, "duration": 0.22 if heavy else 0.12, "attack": 0.002, "release": 2.4},
        {"kind": "saw", "freq": 390 if heavy else 760, "sweep": -180, "amp": 0.22, "duration": 0.08, "release": 3.2},
        {"kind": "sine", "freq": 1450 if heavy else 2100, "sweep": -650, "amp": 0.18, "duration": 0.045, "release": 2.0},
    ], seed=seed)


def write_block(path: Path):
    write_layered_wav(path, 0.2, [
        {"kind": "noise", "amp": 0.42, "duration": 0.038, "release": 5.0},
        {"kind": "square", "freq": 260, "sweep": 80, "amp": 0.28, "duration": 0.075, "release": 2.2},
        {"kind": "sine", "freq": 920, "sweep": -240, "amp": 0.32, "duration": 0.12, "release": 3.4},
        {"kind": "sine", "freq": 1840, "sweep": -620, "amp": 0.16, "duration": 0.08, "release": 2.0},
    ], seed=177)


def write_ui(path: Path, confirm=False, back=False):
    base = 720 if confirm else 460 if back else 620
    write_layered_wav(path, 0.08 if not confirm else 0.14, [
        {"kind": "sine", "freq": base, "sweep": 240 if confirm else -160 if back else 180, "amp": 0.32, "duration": 0.07 if not confirm else 0.12, "release": 1.6},
        {"kind": "sine", "freq": base * 1.5, "sweep": 120, "amp": 0.12, "start": 0.018, "duration": 0.055, "release": 1.4},
    ], seed=300 + base)


def write_announcer_stinger(path: Path, phrase: str):
    phonemes = {
        "round_start": [(120, 0.16), (155, 0.14), (210, 0.18), (170, 0.22)],
        "fight": [(180, 0.11), (260, 0.08), (140, 0.16)],
        "ko": [(95, 0.2), (72, 0.32)],
        "perfect": [(170, 0.11), (220, 0.1), (190, 0.1), (240, 0.12), (160, 0.2)],
        "rage_ready": [(110, 0.18), (150, 0.18), (235, 0.28)],
    }[phrase]
    layers = []
    cursor = 0.0
    for freq, length in phonemes:
        layers.append({"kind": "formant", "freq": freq, "amp": 0.42, "start": cursor, "duration": length, "attack": 0.018, "release": 1.1, "partials": [(1.0, 1.0), (1.8, 0.35), (2.7, 0.2), (4.2, 0.12)]})
        layers.append({"kind": "noise", "amp": 0.055, "start": cursor, "duration": length * 0.8, "attack": 0.02, "release": 1.4})
        cursor += length * 0.78
    layers.append({"kind": "sine", "freq": 54, "amp": 0.22, "start": 0.0, "duration": cursor + 0.18, "release": 1.8})
    write_layered_wav(path, cursor + 0.24, layers, seed=700 + len(phrase))


def write_super(path: Path):
    write_layered_wav(path, 1.05, [
        {"kind": "sine", "freq": 78, "sweep": 260, "amp": 0.35, "duration": 0.95, "release": 0.9},
        {"kind": "saw", "freq": 210, "sweep": 920, "amp": 0.24, "duration": 0.88, "release": 0.8},
        {"kind": "noise", "amp": 0.12, "start": 0.15, "duration": 0.72, "release": 0.7},
        {"kind": "sine", "freq": 1200, "sweep": 1800, "amp": 0.18, "start": 0.35, "duration": 0.42, "release": 1.1},
    ], seed=9191)


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
    write_impact(SOURCE_ART / "Audio" / "impact_light.wav", heavy=False, seed=101)
    write_impact(SOURCE_ART / "Audio" / "impact_heavy.wav", heavy=True, seed=102)
    write_block(SOURCE_ART / "Audio" / "block_guard.wav")
    write_super(SOURCE_ART / "Audio" / "super_riser.wav")
    write_ui(SOURCE_ART / "Audio" / "menu_tick.wav")
    write_ui(SOURCE_ART / "Audio" / "menu_confirm.wav", confirm=True)
    write_ui(SOURCE_ART / "Audio" / "menu_back.wav", back=True)
    write_announcer_stinger(SOURCE_ART / "Audio" / "round_start.wav", "round_start")
    write_announcer_stinger(SOURCE_ART / "Audio" / "announcer_fight.wav", "fight")
    write_announcer_stinger(SOURCE_ART / "Audio" / "announcer_ko.wav", "ko")
    write_announcer_stinger(SOURCE_ART / "Audio" / "announcer_perfect.wav", "perfect")
    write_layered_wav(SOURCE_ART / "Audio" / "ko_hit.wav", 0.72, [
        {"kind": "noise", "amp": 0.78, "duration": 0.09, "release": 5.2},
        {"kind": "sine", "freq": 58, "sweep": -18, "amp": 0.9, "duration": 0.62, "release": 2.0},
        {"kind": "saw", "freq": 96, "sweep": -40, "amp": 0.24, "duration": 0.32, "release": 2.4},
    ], seed=808)
    write_layered_wav(SOURCE_ART / "Audio" / "wall_splat.wav", 0.38, [
        {"kind": "noise", "amp": 0.72, "duration": 0.08, "release": 4.0},
        {"kind": "sine", "freq": 84, "sweep": -52, "amp": 0.78, "duration": 0.32, "release": 2.1},
        {"kind": "square", "freq": 180, "sweep": -60, "amp": 0.18, "duration": 0.11, "release": 3.0},
    ], seed=809)
    write_layered_wav(SOURCE_ART / "Audio" / "counter_hit.wav", 0.26, [
        {"kind": "sine", "freq": 1460, "sweep": 780, "amp": 0.42, "duration": 0.09, "release": 2.0},
        {"kind": "noise", "amp": 0.4, "duration": 0.04, "release": 4.2},
        {"kind": "sine", "freq": 118, "sweep": -24, "amp": 0.44, "duration": 0.2, "release": 2.0},
    ], seed=810)
    write_layered_wav(SOURCE_ART / "Audio" / "throw_hit.wav", 0.34, [
        {"kind": "noise", "amp": 0.46, "duration": 0.05, "release": 3.8},
        {"kind": "sine", "freq": 118, "sweep": -30, "amp": 0.72, "duration": 0.3, "release": 2.1},
        {"kind": "saw", "freq": 250, "sweep": -110, "amp": 0.18, "duration": 0.13, "release": 2.5},
    ], seed=811)
    write_announcer_stinger(SOURCE_ART / "Audio" / "rage_ready.wav", "rage_ready")
    character_blockout("zara_vey", bulky=False)
    character_blockout("hamza_kade", bulky=True)
    stage_assets()
    manifests()


if __name__ == "__main__":
    main()
