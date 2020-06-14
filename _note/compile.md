# ffmpeg、wasm 编译相关

## ffmpeg 编译流程

### 最基本编译

```
1. github  clone  ffmpeg仓库

2. 根目录下执行./configure

3. make -j8

4. make install
```

### 编译 ffmpeg 静态库

> 要得到最小功能集,只用于解封装 ts、mp4,解码 h265、hevc 的静态库,可参考[ffmpeg configure](https://github.com/FFmpeg/FFmpeg/blob/master/configure),取消不必要的功能，只启用如下功能,编译生成`libavcodec.a , libavformat.a , libswscale.a libavutil.a`几个静态库

```c
#!/bin/sh

./configure --prefix=$(pwd)/../ffmpeg_api_use_demo/libs1 \
--cpu=generic --target-os=none --arch=x86_64 \
--enable-small \
--extra-cflags=-Os \ // 启用编译优化
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
--enable-parser=hevc \ // 不启用hevc的parser,对有些1080p的h265视频解码存在绿屏问题
--enable-protocol=data \ // 允许使用AVIOContext的方式(读buffer) 处理视频
--enable-protocol=file \ // 允许读取本地文件方式处理视频
--enable-decoder=aac --enable-decoder=h264 --enable-decoder=hevc \
--enable-demuxer=mov --enable-demuxer=mpegts

```

如上脚本放在 FFmpeg 根目录下执行:

```c
sh  configure.sh
make -j8
make install
```

可在`ffmpeg_api_use_demo`目录下生成`libs1`文件夹,其下包含生成的头文件和静态库

### pthread

以上编译的版本是使用 pthread 的,不使用 pthread 的话要指定`--disable-pthread`

**使用 pthread 的话需要在代码中指定使用的线程数量,是在分配视频解码器上下文时,打开解码器之前需指定:**

```c
AvFormatContext *fmtCtx =NULL;
AVCodecParameters *encodeParams = NULL;
AVStream *stream = NULL;
AvCodec *decoder = NULL;
AvCodecContext *decodeCtx = NULL;
int streamIndex;

avformat_open_input(&fmtCtx,argv[1],NULL,NULL);
streamIndex = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &decoder, 0);
encodeParams = fmtCtx->streams[streamIndex]->codecpar;
decodeCtx = avcodec_alloc_context3(decoder);
avcodec_parameters_to_context(decodeCtx, encodeParams);
decodeCtx->thread_count = 3;// !!!!!
avcodec_open2(decodeCtx, decoder, NULL);

```

### debug ffmpeg 源码

编译时指定

```c
--extra-cflags=-g \
--extra-ldflags=-g \
```

生成可 debug 进 ffmpeg 源码的静态库

vscode 中配置:

1. 安装 codeLLDB 插件 (macos)

```js

/Applications/Visual\ Studio\ Code.app/Contents/Resources/app/bin/code --install-extension path_of_codelldb-x86_64-darwin.vsix

```

2. .vscode/launch.json

```js
{
  "version": "0.2.0",
  "configurations": [
    {
      "name": "debug",
      "type": "lldb",
      "request": "launch",
      "program": "${workspaceFolder}/demo",
      "args": ["${workspaceFolder}/_media/1.flv"],
      "stopOnEntry": false,
      "cwd": "${workspaceFolder}",
      "internalConsoleOptions": "openOnSessionStart",
      "terminal": "external",
      "preLaunchTask": "make"
    }
  ]
}

.vscode/tasks.json

{
  "tasks": [
    {
      "type": "shell",
      "label": "make",
      "command": "make"
    }
  ],
  "version": "2.0.0"
}


```

### 静态库使用

源代码编译时指定依赖的 ffmepg 静态库头文件及 libx 位置

```
clang  ff_base/base.c ff_decode_video/decode_video.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base -v -g -o demo
```

## wasm 编译流程

可分两步,1: 生成使用 emcc 编译的 ffmpeg 静态库 2. 源代码使用 emcc 编译生成 wasm

### 使用 emcc 编译 ffmpeg

```
只需替换如下部分:

emconfigure ./configure --prefix=$(pwd)/../ffmpeg_api_use_demo/libs2 \
--cc="emcc" --cxx="em++" --ar="emar"

其他编译选项同上
```

### 编译 wasm

[emcc settings.js](https://github.com/emscripten-core/emscripten/blob/master/src/settings.js)

```c
emcc \
ff_wasm/decode.c \
ff_base/base.c \
-lavutil  -lavformat -lavcodec  -lswscale \
-Llibs2/lib -Ilibs2/include -Iff_base \
-Os --closure 1 \ // 编译优化 并压缩生成的.js文件
-s ENVIRONMENT=worker \
-s MODULARIZE=1 \ // 导出function
-s EXPORT_NAME=m \  // 导出的函数名
-s ALLOW_MEMORY_GROWTH=1 \
-s TOTAL_MEMORY=16777216 \
-s EXPORTED_FUNCTIONS='["_init"]' \ // 暴露的c接口函数
-s EXTRA_EXPORTED_RUNTIME_METHODS='["addFunction"]' \ // 允许js传递 回调函数给c代码用,通过Module.addFunction() 运行时添加
-s RESERVED_FUNCTION_POINTERS=20 \
-o libs/decoder.js

```

以上指定了

```
-s MODULARIZE=1 \
-s EXPORT_NAME=m \

这两个还是比较重要的,使生成的js结构如下:
//   var EXPORT_NAME = function(Module) {
//     Module = Module || {};
//     // .. all the emitted code from emscripten ..
//     return Module;
//   };
```

### wasm 使用

emcc 编译源代码生成`.js`文件和`.wasm` 文件,在浏览器端可以在 worker 中运行 wasm 进行视频解码,不阻塞 main thread

伪代码:

```javascript
let context = self;

export default function() {
  function bootWasm() {
    context.importScripts('cdn path of js');
    context.Module = context.m({
      locateFile: () => {
        return 'cdn path of wasm';
      }
      // wasmBinary:()=>{

      // }
    }); // 这里m function 是编译wasm时指定的 -s EXPORT_NAME=m
    context.onRuntimeInitialized = () => {
      context.postMessage({ type: 'wasmInit' });
    };
  }

  bootWasm();

  context.addEventListener("message",e=>{
    const {type,data} = e.data;
    switch(type){
      case "initCallback":
          Module._init_callback(xxxxx) //调用c暴露的接口
          break;
      case "buffer":
          // xxxx
          break;
      .
      .
      .
      .
    }
  })

}
```

需要注意的是一般编译生成的 wasm 存放在 cdn 上,而生成的`.js`文件中对`.wasm`的加载指定的是一个写死的默认.wasm 文件名,**可以通过传递给 m()一个默认的 Module 对象,指定一个 locationFile 方法,在这个方法里返回 wasm 的真实 cdn 地址**

另外生成的 wasm 文件体积还是相对较大的,只解封转 ts,解码 aac、hevc 编译的 wasm 文件也有 1.2M,**可以对 wasm 做一些缓存策略,存储在 indexDB 等等,然后通过 wasmBinary 方法自己处理读取本地 indexDB 中 wasm 文件的 buffer 逻辑**
