# Maelir
Maelir is a round screen GPS plotter for classic boats. It's based on an ESP32S3 and an
ESP32C3, both from waveshare, plus a UBLOX Neo-7M GPS module and a rotary encoder.

![the hardware on the desk](doc/maelir_open.jpg)
![the GPS plotter in the boat](doc/finished_product.jpg)

-----
The rest of the document is just for me to remember commands, for now. TODO: More description...

The common parts (threads, HAL, drivers, menu etc) are in [libmaelir](https://github.com/SimonKagstrom/libmaelir),
which is a submodule. Python dependencies:

```
pip3 install -r requirements.txt
```

Qt:

```
cmake -B maelir_qt -GNinja -DCMAKE_BUILD_TYPE=Debug ~/projects/maelir/qt/
maelir_qt/maelir_qt -m <map.bin>
```

Unittest:

```
cmake -B maelir_unittest -GNinja -DCMAKE_BUILD_TYPE=Debug ~/projects/maelir/test/unittest
ctest --test-dir maelir_unittest
```


Target (qualia_esp32s3, waveshare_lcd_esp32p4 or the waveshare_io_board_esp32c3/h2):

```
source $HOME/.espressif/tools/activate_idf_v6.1.sh

cmake -GNinja -B maelir_qualia_esp32s3/ -DCMAKE_BUILD_TYPE=Release ~/projects/maelir/esp32/qualia_esp32s3
```

Create the map data:
```
ulimit -n 65536
tools/image_save.py image_cache out_dir <path-to-saved-har-json-file>
<qt-build>/map_editor maelir_metadata.yaml
tools/tiler.py maelir_metadata.yaml map.bin
```

Flash the map:
```
esptool.py write_flash --flash_mode dio --no-compress --flash_freq 40m --flash_size 16MB 0x00400000 map_data.bin
```

Flash the op board:
```
python -m esptool write_flash @flash_project_args && python -m esp_idf_monitor -p /dev/tty.wchusbserial59710824481 ./maelir_waveshare_io_board_esp32h2.elf
```
