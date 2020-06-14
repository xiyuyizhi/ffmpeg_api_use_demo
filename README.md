# ffmpeg_api_use_demo

- [基本解封装、解码流程](./_note/basic.md)

- [关于提取音频 pcm 数据、视频 yuv 数据](./_note/decode.md)

- [AVIOContext 自定义 IO 和 seek](./_note/avio_context.md)

- [ffmpeg 编译、emscripten 使用相关](./_note/compile.md)

- [thread 在 WebAssembly 中使用](./_note/thread.md)

- [flv 直播流 处理](./_note/live.md)

- [remux](./_note/remux.md)

## run

```
 1. // 编译ffmpeg 静态库
  sudo sh scripts/compile_libx.sh

 2. make // 修改Makefile, 指定target 生成可执行文件

 3. ./demo _media/xx.ts xxx

 4. 启动服务 sever/rgb.html等 可查看解码视频帧的rgb图像、可播放解码音频的pcm数据

```

## run wasm

```
  先注释掉 base.h中 #define MAIN_RUN

  1. // 编译用于webassembly 的ffmpeg 静态库
   sudo sh scripts/comile_libx_emcc.sh

  2. // build wasm
   sudo sh scripts/build_xxxx

```
