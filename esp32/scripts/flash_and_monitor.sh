#!/bin/sh

python $IDF_PATH/components/esptool_py/esptool/esptool.py write_flash @flash_project_args
if [ $? -eq 0 ]; then
    python -m esp_idf_monitor ./*.elf
fi
