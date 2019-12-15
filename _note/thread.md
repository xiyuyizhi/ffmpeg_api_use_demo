# thread 在 WebAssembly 中使用

> webassembly 中使用 thread,需要浏览器支持 SharedArrayBuffer, chrome70 开始 chrome 支持 webassembly 中使用 thread,不过默认不开启,需要自己设置 chrome://flags,从 chrome74 开始默认支持了 webassembly 中使用 thread

> 对于 ffmpeg 软解,即便是直接执行 c 的可执行文件,如果不开启 pthread,不使用硬件加速,解码 1080p 的 h265 视频,解码速度依然比较勉强

各种情况解码速度对比(MacBook Pro Intel Core i7)

| 分辨率    | with thread(3) | no thread |
| --------- | -------------- | --------- |
| 720p 265  | 80fps          | 40fps     |
| 1080p 265 | 35fps          | <20fps    |

(同清晰度,码率差距较大,解码效率也差距较大)

`经测试: 编译成 wasm 运行在浏览器端时,如果不用多线程,mac 下对中等码率 720p h265 视频解码效率还是基本能满足的,使用多线程(3 个)后,720p 完全没问题(需占用 10%的 cpu),对 1080p,也基本能满足正常播放(需占用 25%的 cpu)`

#### ffmpeg 解码使用 pthread

首先需要编译带 pthread 功能的静态库

然后在打开解码器之前在解码器上下文上指定使用的线程数量

```c
AVCodecContext * ctx = avcodec_alloc_context3(decode);
ctx.thread_count = 2;
```

#### 编译 使用 thread 的 wasm

```c
emcc \
  ff_wasm/decode.c \
  ff_base/base.c \
  -lavutil  -lavformat -lavcodec  -lswscale \
  -Llibs2/lib -Ilibs2/include -Iff_base \
  -Os \
  -s USE_PTHREADS=1 \  //允许使用线程
  -s PTHREAD_POOL_SIZE=2 \ //对应创建几个worker,wasm中使用多线程,其实就是通过worker+sharedArrayBuffer。对ffmpeg方法实现中使用thread实现的部分,在浏览器端运行时,变成对应的几个worker执行
  -s MODULARIZE=1 \
  -s EXPORT_NAME=m \
  -s TOTAL_MEMORY=251658240 \ // 在编译不使用thread的wasm版本时,total_memory可以指定一个较小的值,然后指定ALLOW_MEMORY_GROWTH,自动增长。但使用thread时,结合ALLOW_MEMORY_GROWTH会有问题,需要TOTAL_MEMORY指定一个相对较大的值
  -s EXPORTED_FUNCTIONS='["_init"]' \ // 暴露的c接口函数
  -s EXTRA_EXPORTED_RUNTIME_METHODS='["addFunction"]' \ // 允许js传递 回调函数给c代码用,通过Module.addFunction() 运行时添加
  -s RESERVED_FUNCTION_POINTERS=20 \
  -o libs/t_decoder.js
```

#### 支持 thread 的 wasm 的使用

emscripten 编译支持 thread 的 wasm,会生成 4 个文件,`decode.js`,`decode.wasm`,`decode.worker.js`,`decode.js.mem`

在自己的 worker 实现中引入 decode.js,decode.js 会自己加载 wasm,.mem,自己再去创建几个 worker 线程,

需要注意的是 decode.js 中创建 worker 直接使用 new Worker('cdn path of decode.worker.js'),存在跨域问题,即便允许 CORS 跨域,由于 Worker 安全限制也不能创建。所以需要自己加载 decode.worker.js 转成 blob 供 decode.js 创建 worker

伪代码:

```javascript
let context = self;
export default function() {

  function bootWasmWithThread() {
    context.importSscipts('cdn path of decode.js');
    fetch('cdn path of decode.worker.js')
      .then(res => res.blob())
      .then(blob => {
        context.Module = context.m({
          locateFile:(suffix)=>{
            if(/\.worker\.js$/.test(suffix)){
              return URL.createObjectURL(blob);
            }
            if(/\.(wasm|js\.mem)$/.test(suffix)){
              return "cdn path of xxxx";
            },
          },
          "mainScriptUrlOrBlob":"cdn path of decode.js"
        }) // 这里m function 是编译wasm时指定的 -s EXPORT_NAME=m

        context.onRuntimeInitialized = () => {
          context.postMessage({ type: 'wasmInit' });
        };
      });
  }

  bootWasmWithThread();

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
