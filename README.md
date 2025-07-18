# Maelir
Maelir is a round screen GPS plotter for classic boats. It's based on an ESP32S3 and an
ESP32C3, both from waveshare, plus a UBLOX Neo-7M GPS module and a rotary encoder.

![the hardware on the desk](doc/maelir_open.jpg)
![the GPS plotter in the boat](doc/finished_product.jpg)

-----
The rest of the document is just for me to remember commands, for now. TODO: More description...


Qt:

```
conan install -of maelir --build=missing -s build_type=Debug ~/projects/maelir/conanfile.txt
cmake -B maelir -GNinja -DCMAKE_PREFIX_PATH="`pwd`/maelir/build/Debug/generators/" -DCMAKE_BUILD_TYPE=Debug ~/projects/maelir/qt/
```

Unittest:

```
conan install -of maelir_unittest --build=missing -s build_type=Debug ~/projects/maelir/conanfile.txt
cmake -B maelir_unittest -GNinja -DCMAKE_PREFIX_PATH="`pwd`/maelir_unittest/build/Debug/generators/" -DCMAKE_BUILD_TYPE=Debug ~/projects/maelir/test/unittest
```


Target:

```
cd <src>/target/qualia_esp32s3
idf.py update-dependencies

conan install -of ~/projects/build/maelir_qualia_esp32s3/ -pr ~/projects/maelir/target/conanprofile.txt --build=missing -s build_type=Release ~/projects/maelir/conanfile-target.txt

cmake -GNinja -B maelir_qualia_esp32s3/ -DCMAKE_PREFIX_PATH="`pwd`/maelir_esp32s3/build/Release/generators/" -DCMAKE_BUILD_TYPE=Release ~/projects/maelir/target/qualia_esp32s3
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
