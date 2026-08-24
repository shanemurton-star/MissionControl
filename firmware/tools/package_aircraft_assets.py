#!/usr/bin/env python3
"""Resize transparent aircraft art and package it for LVGL's PNG decoder."""

from __future__ import annotations

import argparse
from collections import deque
from pathlib import Path

from PIL import Image


def remove_light_border_background(image: Image.Image) -> Image.Image:
    """Remove an opaque near-white matte connected to the image border."""
    image = image.convert("RGBA")
    width, height = image.size
    pixels = image.load()
    visited: set[tuple[int, int]] = set()
    queue: deque[tuple[int, int]] = deque()

    def background_candidate(x: int, y: int) -> bool:
        red, green, blue, _ = pixels[x, y]
        return min(red, green, blue) >= 235 and max(red, green, blue) - min(red, green, blue) <= 14

    for x in range(width):
        queue.append((x, 0))
        queue.append((x, height - 1))
    for y in range(height):
        queue.append((0, y))
        queue.append((width - 1, y))

    while queue:
        x, y = queue.popleft()
        if (x, y) in visited or not background_candidate(x, y):
            continue
        visited.add((x, y))
        if x > 0: queue.append((x - 1, y))
        if x + 1 < width: queue.append((x + 1, y))
        if y > 0: queue.append((x, y - 1))
        if y + 1 < height: queue.append((x, y + 1))

    for x, y in visited:
        red, green, blue, _ = pixels[x, y]
        pixels[x, y] = (red, green, blue, 0)
    return image


def package(
    source: Path,
    name: str,
    asset_dir: Path,
    include_dir: Path,
    remove_light_background: bool,
) -> None:
    image = Image.open(source).convert("RGBA")
    if remove_light_background:
        image = remove_light_border_background(image)
    alpha = image.getchannel("A").point(lambda value: 255 if value > 16 else 0)
    bounds = alpha.getbbox()
    if bounds is None:
        raise ValueError(f"{source} has no visible pixels")

    image = image.crop(bounds)
    scale = min(280 / image.width, 160 / image.height, 1.0)
    size = (max(1, round(image.width * scale)), max(1, round(image.height * scale)))
    image = image.resize(size, Image.Resampling.LANCZOS)

    asset_path = asset_dir / f"{name}.png"
    asset_path.parent.mkdir(parents=True, exist_ok=True)
    image.save(asset_path, format="PNG", optimize=True)

    data = asset_path.read_bytes()
    symbol = f"aircraft_{name}_png"
    lines = [f"static const uint8_t {symbol}[] = {{"]
    for offset in range(0, len(data), 12):
        values = ", ".join(f"0x{value:02x}" for value in data[offset:offset + 12])
        lines.append(f"  {values},")
    lines.extend([
        "};",
        f"static const uint32_t {symbol}_len = {len(data)};",
        "",
    ])
    include_path = include_dir / f"aircraft_{name}.inc"
    include_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"{name}: {image.width}x{image.height}, {len(data)} bytes")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("name")
    parser.add_argument("source", type=Path)
    parser.add_argument("--asset-dir", type=Path, default=Path("assets/aircraft"))
    parser.add_argument("--include-dir", type=Path, default=Path("src/ui"))
    parser.add_argument("--remove-light-background", action="store_true")
    args = parser.parse_args()
    package(
        args.source,
        args.name,
        args.asset_dir,
        args.include_dir,
        args.remove_light_background,
    )


if __name__ == "__main__":
    main()
