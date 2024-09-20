#!/usr/bin/env python3

import json
from haralyzer import HarParser, HarPage

import time
import os

from sweden_crs_transformations.crs_projection import CrsProjection
from sweden_crs_transformations.crs_coordinate import CrsCoordinate

from PIL import Image

import sys


class Position:
    def __init__(self, latitude, longitude):
        self.longitude = longitude
        self.latitude = latitude

    def __str__(self) -> str:
        return "({}, {})".format(self.latitude, self.longitude)

    def __eq__(self, value: object) -> bool:
        return self.longitude == value.longitude and self.latitude == value.latitude


class SjofartsverketEntry:
    #
    # x------.    Top left
    # |      |
    # |      |    Bottom right (top left on the lower)
    # `------y------.
    #        |      |
    #        |      |
    #        `------y    Bottom right
    #
    #
    def __init__(self, entry, cache_dir: str):
        bbox = None
        for key in entry["queryString"]:
            if key["name"] == "BBOX":
                bbox = key["value"]
                break

        self.bbox = bbox
        bbox = bbox.split("%2C")

        corners = []
        corners_sweref = []
        for i in range(0, len(bbox), 2):
            longitude = float(bbox[i + 1])
            latitude = float(bbox[i])
            as_SWEREF: CrsCoordinate = CrsCoordinate.create_coordinate(
                CrsProjection.SWEREF_99_TM, latitude, longitude
            )
            corners_sweref.append([latitude, longitude])
            corners.append(
                [
                    float(as_SWEREF.transform(CrsProjection.WGS84).get_latitude_y()),
                    float(as_SWEREF.transform(CrsProjection.WGS84).get_longitude_x()),
                ]
            )

        self.top_left = Position(corners[0][0], corners[0][1])
        self.bottom_right = Position(corners[1][0], corners[1][1])

        # More or less...
        self.top_right = Position(self.top_left.latitude, self.bottom_right.longitude)
        self.bottom_left = Position(self.bottom_right.latitude, self.top_left.longitude)

        self.longitude_size = abs(self.top_left.longitude - self.bottom_right.longitude)
        self.latitude_size = abs(self.top_left.latitude - self.bottom_right.latitude)

        self.tiles_around = {
            (0, 1): None, # below
            (1, 0): None, # right
            (0, -1): None, # top
            (-1, 0): None, # left
        }

        self.center_position = Position(
            (self.top_left.latitude + self.bottom_right.latitude) / 2,
            (self.top_left.longitude + self.bottom_right.longitude) / 2,
        )

        self.url = entry["url"]
        self.cache_entry = os.path.join(
            cache_dir,
            "{}.png".format(self.bbox),
        )

        if not os.path.exists(self.cache_entry):
            print("Downloading {} to {}".format(self.url, self.cache_entry))
            rv = os.system("curl -s -o {} '{}'".format(self.cache_entry, self.url))
            if rv != 0:
                print("Failed to download {}".format(self.url))
                sys.exit(1)
            time.sleep(0.1)
        #            data = session.get(self.url).content
        #            with open(self.cache_entry, 'wb') as f:
        #                f.write(data)
        #                f.close()

        self.image = Image.open(self.cache_entry)
        assert self.image.size[0] == 256
        assert self.image.size[1] == 256

        #        print("E: {} {}".format(self.bottom_right, self.top_left))
        self.x = None
        self.y = None

        self.next = None
        self.prev = None

    def place(self, x, y):
        self.x = x
        self.y = y

    def direction_of_other(self, other):
        # Other is to the right
        if abs(other.top_left.longitude - self.top_right.longitude) < self.longitude_size * 0.05 and abs(other.top_left.latitude - self.top_right.latitude) < self.latitude_size * 0.05:
            return (1, 0)
        # Other is to the left
        if abs(other.top_right.longitude - self.top_left.longitude) < self.longitude_size * 0.05 and abs(other.top_right.latitude - self.top_left.latitude) < self.latitude_size * 0.05:
            return (-1, 0)
        # Other is below
        if abs(other.top_left.longitude - self.bottom_left.longitude) < self.longitude_size * 0.05 and abs(other.top_left.latitude - self.bottom_left.latitude) < self.latitude_size * 0.05:
            return (0, -1)
        # Other is above
        if abs(other.bottom_left.longitude - self.top_left.longitude) < self.longitude_size * 0.05 and abs(other.bottom_left.latitude - self.top_left.latitude) < self.latitude_size * 0.05:
            return (0, 1)


        return None

    def place_other(self, other):
        direction = self.direction_of_other(other)
        if direction is None:
            return

        self.tiles_around[direction] = other
        other.tiles_around[(-direction[0], -direction[1])] = self

    def is_resolved(self):
        return all(v is not None for v in self.tiles_around.values())

    def resolve_neighbors(self, x, y):
        if x is not None:
            if self.x is not None:
                # Already resolved
                return

            self.x = x

            for key, neighbor in self.tiles_around.items():
                if neighbor is None:
                    continue
                dx,dy = key
                neighbor.resolve_neighbors(x + dx, None)
        if y is not None:
            if self.y is not None:
                # Already resolved
                return

            self.y = y

            for key, neighbor in self.tiles_around.items():
                if neighbor is None:
                    continue
                dx,dy = key
                neighbor.resolve_neighbors(None, y + dy)


    def __str__(self) -> str:
        return "({}, {}) -> ({}, {})".format(
            self.top_left, self.bottom_right, self.x, self.y
        )


class OrderTiles:
    def __init__(self, entries):
        self.entries = entries

        self.long_size = abs(
            self.entries[0].top_left.longitude - self.entries[0].bottom_right.longitude
        )
        self.lat_size = abs(
            self.entries[0].top_left.latitude - self.entries[0].bottom_right.latitude
        )

        # the lowest longitude and highest latitude in the list
        lowest_longitude = min(entry.bottom_right.longitude for entry in self.entries)
        highest_longitude = max(entry.bottom_right.longitude for entry in self.entries)
        highest_latitude = max(entry.bottom_right.latitude for entry in self.entries)

        width = 0
        height = 0
        entries_by_position = {}
        print(lowest_longitude, highest_latitude)
        empties = []

        # Sort entries by latitude and longitude
        sorted_entries = sorted(self.entries, key=lambda e: (e.center_position.latitude, e.center_position.longitude))

        width = 0
        height = 0


        resolved = []
        unresolved = sorted_entries

        for entry in unresolved:
            if entry.is_resolved():
                continue

            for other in sorted_entries:
                if entry == other:
                    continue

                entry.place_other(other)
                if entry.is_resolved():
                    resolved.append(entry)
                    break


        lowest_longitude_entry = min(resolved, key=lambda e: e.top_left.longitude)
        lowest_latitude_entry = max(resolved, key=lambda e: e.bottom_right.latitude)

        lowest_longitude_entry.resolve_neighbors(0, None)
        lowest_latitude_entry.resolve_neighbors(None, 0)

        width = 0
        height = 0
        for entry in resolved:
            if entry.x is not None:
                width = max(width, entry.x * 256)
            if entry.y is not None:
                height = max(height, entry.y * 256)

        print(width, height)
        img = Image.new("RGB", (width, height), (255,0,0))

        for entry in self.entries:
            if entry.x is not None and entry.y is not None:
                img.paste(entry.image, (entry.x * 256, entry.y * 256))

        img.save("test.png")

# Download the .har file from Developer tools(roughly the same as your operations), and we can parse it offline.
# Even if we have many image files to be download, it will not take too much time to wait to paste.
if __name__ == "__main__":
    if len(sys.argv) < 4:
        print(
            "Usage: {} <cache_directory> <out_directory> <har_file ...>".format(
                sys.argv[0]
            )
        )
        sys.exit(1)
    cache_dir = sys.argv[1]
    out_dir = sys.argv[2]
    entries = []

    # Muhahaha!
    sys.setrecursionlimit(10000)

    try:
        os.mkdir(cache_dir)
        os.mkdir(out_dir)
    except:
        pass

    for har_file in sys.argv[3:]:

        with open(har_file, "r") as f:
            har_parser = HarParser(json.loads(f.read()))

        data = har_parser.har_data["entries"]

        for entry in data:
            if entry["response"]["content"]["mimeType"].find("image/") == 0:
                entries.append(SjofartsverketEntry(entry["request"], cache_dir))

    tile_orderer = OrderTiles(entries)
