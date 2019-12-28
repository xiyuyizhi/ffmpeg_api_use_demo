#!/bin/sh


path=$(pwd);

if [  `echo $path | grep "\/scripts"` ];then
  echo "根目录下执行脚本"
else
  emcc \
  ff_remux/remux.c \
  ff_base/base.c \
  -lavutil  -lavformat -lavcodec  \
  -Llibs3/lib -Ilibs3/include -Iff_base \
  -Os --closure 1 \
  -s ENVIRONMENT=worker \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=m \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s TOTAL_MEMORY=16777216 \
  -s EXPORTED_FUNCTIONS='["_init_buffer","_process"]' \
  -s EXTRA_EXPORTED_RUNTIME_METHODS='["addFunction"]' \
  -s RESERVED_FUNCTION_POINTERS=20 \
  -o ff_remux/remux.js
fi