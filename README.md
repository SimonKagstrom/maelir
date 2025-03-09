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
<qt-build>/map_editor out_dir/test.png maelir_metadata.yaml
tools/tiler.py maelir_metadata.yaml map.bin
```

Flash the map:
```
esptool.py write_flash --flash_mode dio --no-compress --flash_freq 40m --flash_size 16MB 0x00200000 map_data.bin
```
