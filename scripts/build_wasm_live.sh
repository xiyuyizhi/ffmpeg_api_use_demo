#!/bin/sh


path=$(pwd);

if [  `echo $path | grep "\/scripts"` ];then
  echo "根目录下执行脚本"
else
  emcc \
  ff_live/flv_live.c \
  -lavformat -lavcodec  -lswscale  -lavutil \
  -Llibs2/lib -Ilibs2/include -Iff_base \
  -Os --closure 1 \
  -s ENVIRONMENT=worker \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=m \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s TOTAL_MEMORY=16777216 \
  -s EXPORTED_FUNCTIONS='["_init_process","_do_demuxer","_do_append_buffer","_decodeVideoFrame","_do_read_pkts","_clean"]' \
  -s EXTRA_EXPORTED_RUNTIME_METHODS='["addFunction"]' \
  -s RESERVED_FUNCTION_POINTERS=20 \
  -o libs/live-decode.js
fi