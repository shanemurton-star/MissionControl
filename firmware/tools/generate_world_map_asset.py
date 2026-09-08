#!/usr/bin/env python3
"""Generate MissionControl's compact offline world-map PNG and C include.

Input is Natural Earth's public-domain ne_110m_land.zip. The shapefile parser
is intentionally self-contained so regenerating this asset does not require a
desktop GIS installation.
"""

from __future__ import annotations

import argparse
import io
import struct
import zipfile
from pathlib import Path

from PIL import Image, ImageDraw


WIDTH = 512
HEIGHT = 256
SCALE = 3


def project(longitude: float, latitude: float) -> tuple[int, int]:
    x = (longitude + 180.0) / 360.0 * (WIDTH * SCALE - 1)
    y = (90.0 - latitude) / 180.0 * (HEIGHT * SCALE - 1)
    return round(x), round(y)


def polygon_parts(shapefile: bytes):
    position = 100
    while position + 8 <= len(shapefile):
        _, content_words = struct.unpack(">2i", shapefile[position:position + 8])
        position += 8
        content_length = content_words * 2
        content = shapefile[position:position + content_length]
        position += content_length
        if len(content) < 44:
            continue
        shape_type = struct.unpack("<i", content[:4])[0]
        if shape_type not in (5, 15, 25):
            continue
        number_of_parts, number_of_points = struct.unpack("<2i", content[36:44])
        parts_start = 44
        points_start = parts_start + number_of_parts * 4
        if points_start + number_of_points * 16 > len(content):
            continue
        starts = list(struct.unpack(
            f"<{number_of_parts}i",
            content[parts_start:points_start],
        ))
        starts.append(number_of_points)
        for index in range(number_of_parts):
            points = []
            for point_index in range(starts[index], starts[index + 1]):
                offset = points_start + point_index * 16
                longitude, latitude = struct.unpack("<2d", content[offset:offset + 16])
                points.append(project(longitude, latitude))
            if len(points) >= 3:
                yield points


def write_include(png_path: Path, include_path: Path) -> None:
    data = png_path.read_bytes()
    lines = ["const uint8_t world_map_png[] = {"]
    for offset in range(0, len(data), 12):
        chunk = data[offset:offset + 12]
        lines.append("  " + ", ".join(f"0x{value:02x}" for value in chunk) + ",")
    lines.extend([
        "};",
        f"const uint32_t world_map_png_len = {len(data)};",
        "",
    ])
    include_path.write_text("\n".join(lines), encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("natural_earth_zip", type=Path)
    parser.add_argument("png_output", type=Path)
    parser.add_argument("include_output", type=Path)
    args = parser.parse_args()

    with zipfile.ZipFile(args.natural_earth_zip) as archive:
        shp_name = next(name for name in archive.namelist() if name.endswith(".shp"))
        shapefile = archive.read(shp_name)

    image = Image.new("RGBA", (WIDTH * SCALE, HEIGHT * SCALE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)

    grid_color = (39, 61, 70, 105)
    for longitude in range(-120, 180, 60):
        x, _ = project(longitude, 0)
        draw.line((x, 0, x, HEIGHT * SCALE), fill=grid_color, width=1 * SCALE)
    for latitude in (-60, -30, 0, 30, 60):
        _, y = project(0, latitude)
        draw.line((0, y, WIDTH * SCALE, y), fill=grid_color, width=1 * SCALE)

    land_fill = (11, 29, 37, 255)
    coastline = (54, 91, 105, 255)
    for points in polygon_parts(shapefile):
        draw.polygon(points, fill=land_fill)
        draw.line(points + [points[0]], fill=coastline, width=1 * SCALE, joint="curve")

    image = image.resize((WIDTH, HEIGHT), Image.Resampling.LANCZOS)
    args.png_output.parent.mkdir(parents=True, exist_ok=True)
    args.include_output.parent.mkdir(parents=True, exist_ok=True)
    image.save(args.png_output, format="PNG", optimize=True)
    write_include(args.png_output, args.include_output)


if __name__ == "__main__":
    main()
