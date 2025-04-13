#!/bin/sh

set -x

esptool.py write_flash --flash_mode dio --no-compress --flash_freq 40m --flash_size 16MB 0x00200000 map_data.bin
