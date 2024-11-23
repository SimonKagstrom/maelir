#!/usr/bin/env python3

import os
import sys
import struct
import io
import yaml

import PIL
from PIL import Image

# Allow huge images
PIL.Image.MAX_IMAGE_PIXELS = 933120000

kGpsTileSize = 256

def get_tile_positions_to_ignore(yaml_data: dict, img: Image, tile_size: int):
    out = {}
    cropped_width = img.size[0] - img.size[0] % tile_size
    cropped_height = img.size[1] - img.size[1] % tile_size

    for entry in yaml_data["all_land_tiles"]:
        out[entry["x_pixel"], entry["y_pixel"]] = True

    to_remove = []
    for key, _ in out.items():
        x, y = key

        has_water = False
        for dx in range(-2, 3):
            for dy in range(-2, 3):
                dx *= tile_size
                dy *= tile_size

                if dx == 0 and dy == 0:
                    continue
                if x + dx < 0 or x + dx >= cropped_width:
                    continue
                if y + dy < 0 or y + dy >= cropped_height:
                    continue
                if (x + dx, y + dy) not in out:
                    has_water = True
                    break
        if has_water:
            to_remove.append(key)

    for key in to_remove:
        del out[key]

    return out


def create_tiles(yaml_data: dict, img: Image, to_ignore: dict, tile_size: int):
    tiles = []
    cropped_width = img.size[0] - img.size[0] % tile_size
    cropped_height = img.size[1] - img.size[1] % tile_size

    for y in range(0, cropped_height, tile_size):
        for x in range(0, cropped_width, tile_size):

            if (x, y) in to_ignore:
                tiles.append(None)
                continue

            tile = img.crop((x, y, x + tile_size, y + tile_size))
            tile = tile.convert(
                mode="P", palette=Image.ADAPTIVE, dither=Image.Dither.NONE, colors=64
            )
            tiles.append(tile)

    return tiles


def create_binary(yaml_data: dict, tiles: list, row_length: int, gps_row_length : int, gps_rows : int, dst_file: str):
    data_size = 0

    tile_size = 0
    for tile in tiles:
        if tile is not None:
            tile_size = tile.size[0]
            break

    assert tile_size != 0

    path_finder_tile_size = yaml_data["path_finder_tile_size"]
    path_finder_row_length = int((row_length * tile_size) / path_finder_tile_size)

    land_mask = b''
    land_mask_yaml_data = yaml_data["land_mask"]
    for i in range(0, len(land_mask_yaml_data)):
        land_mask += struct.pack("<I", land_mask_yaml_data[i])

    land_only_tile = Image.new("P", (tile_size, tile_size), 0)
    r = yaml_data["land_pixel_colors"][0]["r"]
    g = yaml_data["land_pixel_colors"][0]["g"]
    b = yaml_data["land_pixel_colors"][0]["b"]

    # Fill land_only_tile with the color of the land (r,g,b)
    for y in range(0, land_only_tile.size[1]):
        for x in range(0, land_only_tile.size[0]):
            land_only_tile.putpixel((x, y), (r, g, b))

    output = io.BytesIO()
    land_only_tile.save(output, format="PNG")
    bytes = output.getvalue()

    land_only_size = len(bytes)

    header_format = "<QffffIIIIIIIIII"
    header_size = struct.calcsize(header_format)
    assert header_size == 64

    # Starts after the MapMetadata header and all FlashTile:s
    land_only_offset = header_size + len(tiles) * 8

    data_size += len(bytes)

    tile_metadata = []
    tile_data = bytes

    current_offset = land_only_offset + len(bytes)
    for index, tile in enumerate(tiles):
        bytes = []

        if tile is None:
            tile_metadata.append((land_only_size, land_only_offset))
            continue

        output = io.BytesIO()
        tile.save(output, format="PNG")
        bytes = output.getvalue()

        # Copy bytes to tile_data
        tile_metadata.append((len(bytes), current_offset))
        tile_data += bytes

        data_size += len(bytes)
        current_offset += len(bytes)

    # Pack metadata and tile_data into the bin_file, little endian format

    bin_file = open(dst_file, "wb")

    # Example data for the header
    magic = 0x54494C5253574654
    tile_count = len(tiles) + 1
    tile_row_size = row_length
    tile_rows = len(tiles) // row_length
    land_mask_row_size = path_finder_row_length
    land_mask_rows = (tile_rows * tile_size) // path_finder_tile_size
    tile_data_offset = header_size  # After the header
    land_mask_data_offset = tile_data_offset + len(tile_metadata) * 8 + len(tile_data)

    # Align the land mask to 4 bytes
    if land_mask_data_offset % 4 != 0:
        land_mask_data_offset += 4 - (land_mask_data_offset % 4)

    gps_data_offset = land_mask_data_offset + len(land_mask)

    lowest_latitude = -200
    highest_latitude = 200
    lowest_longitude = -200
    highest_longitude = 200
    for entry in yaml_data["point_to_gps_position"]:
        if entry["latitude"] == 0 or entry["longitude"] == 0:
            continue

        if entry["latitude"] < lowest_latitude:
            lowest_latitude = entry["latitude"]
        if entry["latitude"] > highest_latitude:
            highest_latitude = entry["latitude"]
        if entry["longitude"] < lowest_longitude:
            lowest_longitude = entry["longitude"]
        if entry["longitude"] > highest_longitude:
            highest_longitude = entry["longitude"]

    # Pack the data into binary format
    header_data = struct.pack(
        header_format,
        magic,

        lowest_longitude,
        lowest_latitude,
        highest_longitude,
        highest_latitude,

        tile_count,
        tile_row_size,
        tile_rows,

        land_mask_row_size,
        land_mask_rows,

        gps_row_length,
        gps_rows,

        tile_data_offset,
        land_mask_data_offset,
        gps_data_offset,
    )

    offset = bin_file.write(header_data)

    assert(offset == tile_data_offset)
    for size, png_offset in tile_metadata:
        cur = bin_file.write(struct.pack("<II", size, png_offset))
        assert(cur == 8)
        offset += cur

    offset += bin_file.write(tile_data)

    # Align the land mask to 4 bytes
    while offset % 4 != 0:
        offset += bin_file.write(b'\xaa')
    assert(offset == land_mask_data_offset)

    bin_file.write(land_mask)

    # Create the GPS data (make sure it's filled with zeroes, and of the right size)
    gps_data = []
    for i in range(0, tile_count):
        gps_data.append([0.0, 0.0])

    for entry in yaml_data["point_to_gps_position"]:
        x = entry["x_pixel"] // kGpsTileSize
        y = entry["y_pixel"] // kGpsTileSize
        index = y * gps_row_length + x

        assert index < len(gps_data)
        gps_data[index] = [entry["latitude"], entry["longitude"]]

    for latitude, longitude in gps_data:
        bin_file.write(struct.pack("<ffff", latitude, longitude))

    return data_size


def save_tiles(tiles: list, num_colors):
    for i, tile in enumerate(tiles):
        tile = tile.convert(
            mode="P", palette=Image.ADAPTIVE, dither=None, colors=num_colors
        )
        tile.save(f"tile_{i}.png", optimize=True, format="PNG")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Usage: {} <input_yaml_file> <output_file>".format(sys.argv[0]))
        sys.exit(1)

    yaml_data = yaml.safe_load(open(sys.argv[1], "r"))

    if "map_filename" not in yaml_data or "tile_size" not in yaml_data:
        print(
            "Error: map_filename/tile_size not found in input yaml file. See mapeditor_metadata.yaml for an example"
        )
        sys.exit(1)

    if "point_to_gps_position" not in yaml_data:
        print(
            "Error: corner_popoint_to_gps_position not found in input yaml file. See mapeditor_metadata.yaml for an example"
        )
        sys.exit(1)

    img = Image.open(yaml_data["map_filename"])

    num_pixels = img.size[0] * img.size[1]
    num_colors = len(img.getcolors(num_pixels))

    tile_size = yaml_data["tile_size"]
    path_finder_tile_size = yaml_data["path_finder_tile_size"]

    to_ignore = get_tile_positions_to_ignore(yaml_data, img, tile_size)

    tiles = create_tiles(yaml_data, img, to_ignore, tile_size=tile_size)
    # save_tiles(tiles, num_colors=256)
    tile_row_length = int(img.size[0] / tile_size)
    gps_row_length = int(img.size[0] / kGpsTileSize)
    gps_rows = int(img.size[1] / kGpsTileSize)

    data_size = create_binary(
        yaml_data,
        tiles,
        row_length=tile_row_length,
        gps_row_length=gps_row_length,
        gps_rows = gps_rows,
        dst_file=sys.argv[2],
    )

    print(
        "tiler: Converted to {} tiles ({} ignored), total size: {:.2f} KiB".format(
            len(tiles), len(to_ignore), data_size / 1024.0
        )
    )
