#!/bin/sh


path=$(pwd);

if [  `echo $path | grep "\/scripts"` ];then
  echo "根目录下执行脚本"
else
  emcc \
  ff_wasm/decode.c \
  ff_base/base.c \
  -lavformat -lavcodec  -lswscale  -lavutil \
  -Llibs2/lib -Ilibs2/include -Iff_base \
  -Os --closure 1 \
  -s ENVIRONMENT=worker \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=m \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s TOTAL_MEMORY=16777216 \
  -s EXPORTED_FUNCTIONS='["_init"]' \
  -s EXTRA_EXPORTED_RUNTIME_METHODS='["addFunction"]' \
  -s RESERVED_FUNCTION_POINTERS=20 \
  -o libs/decoder.js
fi