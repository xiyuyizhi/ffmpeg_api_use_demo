#!/bin/sh

./configure --prefix=$(pwd)/../ffmpeg_api_use_demo/libs1 \
--cpu=generic --target-os=none --arch=x86_64 \
--enable-small \
--extra-cflags=-Os \
--enable-cross-compile \
--disable-inline-asm \
--disable-ffmpeg \
--disable-ffplay \
--disable-ffprobe \
--disable-programs \
--disable-doc \
--disable-htmlpages \
--disable-manpages \
--disable-podpages \
--disable-txtpages \
--disable-x86asm \
--disable-devices \
--disable-avdevice \
--disable-swresample \
--disable-avfilter \
--disable-logging \
--disable-videotoolbox \
--disable-postproc \
--disable-os2threads \
--disable-w32threads \
--disable-network \
--disable-debug \
--disable-everything \
--enable-parser=hevc \
--enable-protocol=data \
--enable-protocol=file \
--enable-decoder=aac --enable-decoder=h264 --enable-decoder=hevc \
--enable-demuxer=mov --enable-demuxer=mpegts

# --extra-cflags=-g \
# --extra-ldflags=-g \