#!/usr/bin/env python3
import json
import math
import random
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageFont

ROOT = Path(__file__).resolve().parents[1]
SOURCE_ART = ROOT / "SourceArt"


def ensure_dirs():
    for sub in ["Textures", "UI/Portraits", "UI/MoveIcons", "UI/Plates", "Data"]:
        (SOURCE_ART / sub).mkdir(parents=True, exist_ok=True)


def save(img, path):
    path.parent.mkdir(parents=True, exist_ok=True)
    img.save(path)


def noise_texture(size, base, accent, seed, wet=False):
    random.seed(seed)
    img = Image.new("RGB", (size, size), base)
    px = img.load()
    for y in range(size):
        for x in range(size):
            grain = random.randint(-22, 22)
            wave = int(math.sin((x * 0.027) + (y * 0.011)) * 14)
            c = tuple(max(0, min(255, base[i] + grain + wave)) for i in range(3))
            px[x, y] = c
    draw = ImageDraw.Draw(img, "RGBA")
    for _ in range(size // 3):
        x = random.randrange(size)
        y = random.randrange(size)
        r = random.randrange(2, 12)
        draw.ellipse((x - r, y - r, x + r, y + r), fill=(*accent, random.randrange(14, 46)))
    if wet:
        for _ in range(80):
            x = random.randrange(size)
            y = random.randrange(size)
            w = random.randrange(30, 130)
            h = random.randrange(3, 18)
            draw.ellipse((x, y, x + w, y + h), fill=(180, 210, 230, random.randrange(16, 42)))
    return img.filter(ImageFilter.SMOOTH_MORE)


def make_textures():
    save(noise_texture(1024, (42, 45, 52), (95, 110, 120), 10, wet=True), SOURCE_ART / "Textures/wet_rooftop_concrete_albedo.png")
    save(noise_texture(1024, (22, 21, 28), (75, 195, 220), 20), SOURCE_ART / "Textures/zara_technical_fabric.png")
    save(noise_texture(1024, (74, 18, 18), (220, 150, 70), 30), SOURCE_ART / "Textures/hamza_embroidered_canvas.png")
    save(noise_texture(1024, (28, 30, 34), (160, 138, 92), 40), SOURCE_ART / "Textures/scuffed_dark_metal.png")

    trim = Image.new("RGBA", (1024, 256), (0, 0, 0, 0))
    d = ImageDraw.Draw(trim, "RGBA")
    for i in range(0, 1024, 64):
        color = (20, 230, 255, 220) if (i // 64) % 2 == 0 else (255, 190, 35, 220)
        d.rounded_rectangle((i + 8, 72, i + 56, 184), radius=14, outline=color, width=6)
        d.line((i + 32, 0, i + 32, 256), fill=(*color[:3], 75), width=3)
    save(trim, SOURCE_ART / "Textures/neon_trim_atlas.png")


def crop_portrait(src, dst, box_ratio):
    img = Image.open(src).convert("RGB")
    w, h = img.size
    left, top, right, bottom = box_ratio
    crop = img.crop((int(w * left), int(h * top), int(w * right), int(h * bottom)))
    crop.thumbnail((512, 512), Image.Resampling.LANCZOS)
    plate = Image.new("RGB", (512, 512), (8, 10, 16))
    plate.paste(crop, ((512 - crop.width) // 2, (512 - crop.height) // 2))
    save(plate, dst)


def make_portraits():
    crop_portrait(SOURCE_ART / "Concepts/zara_vey_concept.png", SOURCE_ART / "UI/Portraits/zara_vey_portrait.png", (0.24, 0.03, 0.78, 0.45))
    crop_portrait(SOURCE_ART / "Concepts/hamza_kade_concept.png", SOURCE_ART / "UI/Portraits/hamza_kade_portrait.png", (0.19, 0.02, 0.81, 0.48))


def make_ui_plates():
    for name, colors in {
        "title_plate": ((7, 9, 15), (40, 220, 255), (255, 196, 40)),
        "versus_plate": ((12, 9, 12), (255, 68, 44), (50, 230, 120)),
        "training_plate": ((8, 12, 16), (110, 180, 255), (255, 255, 255)),
    }.items():
        bg, c1, c2 = colors
        img = Image.new("RGBA", (1024, 256), (*bg, 235))
        d = ImageDraw.Draw(img, "RGBA")
        for x in range(0, 1024, 24):
            a = int(22 + 34 * (math.sin(x * 0.027) + 1) * 0.5)
            d.line((x, 0, x + 160, 256), fill=(*c1, a), width=3)
        d.rounded_rectangle((18, 18, 1006, 238), radius=18, outline=(*c2, 170), width=3)
        d.line((36, 214, 988, 214), fill=(*c1, 120), width=4)
        save(img, SOURCE_ART / f"UI/Plates/{name}.png")

    logo = Image.new("RGBA", (1400, 360), (0, 0, 0, 0))
    d = ImageDraw.Draw(logo, "RGBA")
    try:
        title_font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial Bold.ttf", 136)
        sub_font = ImageFont.truetype("/System/Library/Fonts/Supplemental/Arial.ttf", 34)
    except Exception:
        title_font = ImageFont.load_default()
        sub_font = ImageFont.load_default()
    d.rounded_rectangle((34, 52, 1366, 278), radius=24, fill=(4, 7, 14, 205), outline=(255, 196, 40, 210), width=5)
    for x in range(80, 1320, 80):
        d.line((x, 72, x + 120, 258), fill=(40, 220, 255, 55), width=4)
    d.text((700, 148), "RAVI RIFT", anchor="mm", font=title_font, fill=(255, 214, 70, 255), stroke_width=3, stroke_fill=(0, 0, 0, 255))
    d.text((700, 252), "ORIGINAL 3D FIGHTING GAME", anchor="mm", font=sub_font, fill=(180, 230, 245, 235))
    save(logo, SOURCE_ART / "UI/Plates/ravi_circuit_logo.png")


def make_move_icon(path, label, color, shape):
    img = Image.new("RGBA", (256, 256), (0, 0, 0, 0))
    d = ImageDraw.Draw(img, "RGBA")
    d.rounded_rectangle((18, 18, 238, 238), radius=28, fill=(8, 10, 16, 220), outline=(*color, 220), width=7)
    if shape == "fist":
        d.ellipse((72, 78, 178, 180), fill=(*color, 210))
        d.rectangle((94, 154, 162, 204), fill=(*color, 210))
    elif shape == "kick":
        d.polygon([(76, 72), (142, 108), (198, 166), (170, 198), (112, 136), (62, 106)], fill=(*color, 210))
    elif shape == "low":
        d.arc((48, 74, 210, 224), start=15, end=180, fill=(*color, 230), width=18)
    elif shape == "throw":
        d.ellipse((58, 74, 112, 128), fill=(*color, 220))
        d.ellipse((144, 74, 198, 128), fill=(*color, 220))
        d.line((88, 132, 170, 178), fill=(*color, 230), width=18)
    elif shape == "super":
        for r in [34, 58, 84]:
            d.ellipse((128 - r, 128 - r, 128 + r, 128 + r), outline=(*color, 230), width=7)
        d.polygon([(128, 42), (154, 118), (232, 128), (154, 154), (128, 232), (102, 154), (24, 128), (102, 118)], fill=(*color, 140))
    d.text((128, 220), label, anchor="mm", fill=(235, 240, 248, 255))
    save(img, path)


def make_move_icons():
    icons = {
        "punch": ("P", (80, 220, 255), "fist"),
        "kick": ("K", (255, 196, 40), "kick"),
        "low": ("L", (70, 230, 130), "low"),
        "throw": ("T", (255, 92, 60), "throw"),
        "special": ("S", (160, 120, 255), "super"),
        "super": ("R", (255, 235, 95), "super"),
    }
    for name, (label, color, shape) in icons.items():
        make_move_icon(SOURCE_ART / f"UI/MoveIcons/{name}.png", label, color, shape)


def make_move_data():
    data = {
        "fighters": {
            "Zara Vey": {
                "style": "Magnetic kickboxing",
                "moves": [
                    ["Volt Jab", "high", 5, 3, 13, 8, 2, 7],
                    ["Relay Cross", "high", 7, 3, 16, 10, 1, 8],
                    ["Neon Roundhouse", "mid", 11, 4, 22, 15, -5, 12],
                    ["Circuit Sweep", "low", 15, 4, 25, 13, -9, 6],
                    ["Skyline Upper", "mid launcher", 16, 3, 28, 12, -13, 18],
                    ["Polarity Kick", "mid special", 13, 6, 30, 20, -8, 15],
                    ["Current Toss", "throw", 8, 2, 36, 22, 0, 26],
                    ["Monsoon Breaker", "rage super", 12, 9, 66, 38, -18, 32],
                ],
            },
            "Hamza Kade": {
                "style": "Heavy pressure grappling",
                "moves": [
                    ["Stone Jab", "high", 6, 3, 14, 9, 1, 8],
                    ["Bazaar Hook", "mid", 10, 4, 19, 13, -3, 12],
                    ["Iron Shin", "mid", 14, 5, 25, 17, -6, 13],
                    ["Foundation Low", "low", 17, 4, 27, 15, -11, 8],
                    ["Forge Lift", "mid launcher", 18, 4, 31, 14, -14, 17],
                    ["Truck Art Ram", "mid special", 19, 6, 34, 24, -10, 18],
                    ["Gatekeeper Slam", "throw", 9, 2, 40, 27, 0, 32],
                    ["Lahore Lockdown", "rage super", 16, 9, 72, 45, -21, 34],
                ],
            },
        },
        "columns": ["name", "level", "startup", "active", "recovery", "damage", "block_adv", "hit_adv"],
    }
    (SOURCE_ART / "Data/move_list.json").write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")


def main():
    ensure_dirs()
    make_textures()
    make_portraits()
    make_ui_plates()
    make_move_icons()
    make_move_data()


if __name__ == "__main__":
    main()
