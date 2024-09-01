Qt:

```
conan install -of maelir --build=missing -s build_type=Debug ~/projects/maelir/conanfile.txt
cmake -B maelir -GNinja -DMAP_YAML_PATH=~/projects/build/maelir_metadata.yaml -DCMAKE_PREFIX_PATH="`pwd`/maelir/build/Debug/generators/" -DCMAKE_BUILD_TYPE=Debug ~/projects/maelir/qt/
```

Target:

```
conan install -of ~/projects/build/maelir_esp32s3/ -pr ~/projects/maelir/target/conanprofile.txt --build=missing -s build_type=Release ~/projects/maelir/conanfile-target.txt

cmake -DMAP_YAML_PATH=~/projects/build/maelir_metadata.yaml -DCMAKE_PREFIX_PATH="`pwd`/build/Release/generators/" -DCMAKE_BUILD_TYPE=Release ~/projects/maelir/target/
```
