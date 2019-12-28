#!/bin/sh


path=$(pwd);

if [  `echo $path | grep "\/scripts"` ];then
  echo "根目录下执行脚本"
else
  emcc \
  ff_remux/remux.c \
  -lavformat -lavcodec -lavutil  \
  -Llibs3/lib -Ilibs3/include \
  -Os \
  -s ENVIRONMENT=worker \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=m \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s TOTAL_MEMORY=16777216 \
  -s EXPORTED_FUNCTIONS='["_init_buffer","_process"]' \
  -s EXTRA_EXPORTED_RUNTIME_METHODS='["addFunction"]' \
  -s RESERVED_FUNCTION_POINTERS=20 \
  -o server/remux.js
fi