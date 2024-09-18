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

        self.top_left_sweref = Position(corners_sweref[0][0], corners_sweref[0][1])
        self.bottom_right_sweref = Position(corners_sweref[1][0], corners_sweref[1][1])

        self.top_left = Position(corners[0][0], corners[0][1])
        self.bottom_right = Position(corners[1][0], corners[1][1])

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
        #            data = session.get(self.url).content
        #            with open(self.cache_entry, 'wb') as f:
        #                f.write(data)
        #                f.close()

        self.image = Image.open(self.cache_entry)
        assert self.image.size[0] == 256
        assert self.image.size[1] == 256

        #        print("E: {} {}".format(self.bottom_right, self.top_left))
        self.x = 0
        self.y = 0

    def place(self, x, y):
        self.x = x
        self.y = y

    def __str__(self) -> str:
        return "({}, {}) -> ({}, {})".format(
            self.top_left, self.bottom_right, self.x, self.y
        )


class OrderTiles:
    def __init__(self, entries):
        self.entries = entries

        self.lat_size = abs(
            self.entries[0].top_left.latitude - self.entries[0].bottom_right.latitude
        )
        self.long_size = abs(
            self.entries[0].top_left.longitude - self.entries[0].bottom_right.longitude
        )

        # the lowest longitude in the list
        lowest_longitude = 10000
        lowest_latitude = -10000
        for x in self.entries:
            lowest_longitude = min(lowest_longitude, x.bottom_right.longitude)
            lowest_latitude = max(lowest_latitude, x.bottom_right.latitude)

        #self.one_connection = None
        #for entry in sorted_by_latitude:
        #    for other in sorted_by_latitude:
        #        if entry == other:
        #            continue
        #        if (
        #            entry.bottom_right.latitude == other.top_left.latitude
        #            and entry.bottom_right.longitude == other.top_left.longitude
        #        ):
        #            self.one_connection = [entry, other]
        #            break
#
        #assert self.one_connection is not None

        width = 0
        height = 0
        entries_by_position = {}
        print(lowest_longitude, lowest_latitude)
        empties = []

        for index in range(0,len(self.entries)):
            entry = self.entries[index]
            #print("E: {} {}".format(entry.bottom_right, entry.top_left))
            x = int(round((entry.center_position.longitude - lowest_longitude) / self.long_size))
            y = int(round((lowest_latitude - entry.center_position.latitude) / self.lat_size))

            entry.place(x, y)

            if (x, y) in entries_by_position:
                other = entries_by_position[(x, y)]
                print("Already have {} vs {} at {},{} (long diff {})".format(other.bottom_right, entry.bottom_right, x, y, other.bottom_right.longitude - entry.bottom_right.longitude))
                continue
            width = max(width, x * entry.image.size[0])
            height = max(height, y * entry.image.size[1])

            #print("X {}. From {}".format(entry, entry.bottom_right.latitude - lowest_latitude))
            entries_by_position[(x, y)] = entry


        print(width, height)
        img = Image.new("RGB", (width, height), (255, 255, 255))

        for entry in self.entries:
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
