#!/bin/sh


path=$(pwd);

if [  `echo $path | grep "\/scripts"` ];then
  echo "根目录下执行脚本"
else
  emcc \
  ff_wasm/decode.c \
  ff_base/base.c \
  -lavutil  -lavformat -lavcodec  -lswscale \
  -Llibs2/lib -Ilibs2/include -Iff_base \
  -Os \
  -s USE_PTHREADS=1 \
  -s PTHREAD_POOL_SIZE=2 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=m \
  -s TOTAL_MEMORY=251658240 \
  -s EXPORTED_FUNCTIONS='["_init"]' \
  -s EXTRA_EXPORTED_RUNTIME_METHODS='["addFunction"]' \
  -s RESERVED_FUNCTION_POINTERS=20 \
  -o libs/t_decoder.js
fi