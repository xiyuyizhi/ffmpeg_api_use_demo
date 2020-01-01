# remux

> 通过 ffmepg 进行转封装,并使用 AVIOContext 来做输入输出。 一个使用场景就是: 目前 hls 多还是以 ts 为封装格式,绝大多数浏览器不能直接播放 ts,目前 ts -> mp4 这块是通过 js 来处理的,可以尝试将转封装功能通过 ffmepg 来完成,编译成 wasm 来提供功能

### 准备工作

编译 ffmepg 带功能: 解封装 ts,转封装为 mp4。ffmeg 编译需要特别指定的选项:

```c
--disable-everything \
--enable-protocol=data \
--enable-decoder=aac --enable-decoder=h264 \
--enable-demuxer=mpegts \
--enable-muxer=mp4 \
--enable-parser=aac \
--enable-parser=h264 \
--enable-bsf=aac_adtstoasc \
--enable-bsf=extract_extradata \
```

### AVIOContext 来做输入输出

> 转封装功能要编译成 wasm 使用,js 和 c 之间的数据交互就是 buffer。传递 ts 分片 buffer 数据 给 c 处理,转封装后的 mp4 buffer 再交由 js 去后序处理

[自定义 IO 作为输入见](./avio_context.md)

**自定义 io 作为输出**

主要实现逻辑是: `预先分配一块大一点的buffer(能容纳 remux 后输出数据,js中维护 buffer的起始指针),然后自定义io上下文并挂载给作为输出的format上下文,输出format上下文进行转封装处理,输出数据通过自定义io上下文输出到 预分配的buffer中`

**iocontext 输出回调**

```c

static int write_packet(void *opaque, uint8_t *buf, int buf_size)
{
  BufferData *bd = (BufferData *)opaque;
  if (bd->size < buf_size)
  {
    printf("leak space for write data! %d , %d\n", bd->size, buf_size);
    return -1;
  }
  memcpy(bd->ptr, buf, buf_size); //把ffmpeg生成的数据写到大buffer
  bd->ptr += buf_size;
  bd->size -= buf_size;
  return buf_size;
}

```

### js 中的主要工作

1. 分配一块大一的 buffer 空间,不断复用,remux 后的 mp4 数据存到这块 buffer 中
2. 从这块 buffer 中读取 mp4 数据

```javascript
let outPtr;
let outputSize;
self.addEventListener('message', e => {
  const { type } = e.data;
  switch (type) {
    case 'malloc':
      outputSize = 1024 * 1024 * 5; // alloc 5M for transfer data
      outPtr = Module._malloc(outputSize);
      Module._init_buffer(outPtr, outputSize);
      console.log('init out buffer,ptr:', outPtr);
      break;

    case 'remux':
      const buffer = e.data.buffer;

      if (buffer.byteLength > outputSize) {
        throw new Error('file too big');
        return;
      }

      const t1 = performance.now();
      const ptr = Module._malloc(buffer.byteLength);
      Module.HEAPU8.set(new Uint8Array(buffer), ptr);

      let usedSize = Module._process(ptr, buffer.byteLength);
      console.log('remux take:', performance.now() - t1);
      if (usedSize < 0) {
        //error
        self.postMessage({ type: 'error', data: 'remux error' });
        return;
      }

      let result = Module.HEAPU8.subarray(outPtr, outPtr + usedSize);
      let mp4 = new Uint8Array(result);
      self.postMessage(
        {
          type: 'mp4',
          data: mp4
        },
        [mp4.buffer]
      );
      Module._free(ptr);
      break;

    case 'free':
      // free when no use
      Module._free(outPtr);
      console.log('free out buffer');
      break;
  }
});
```

**详细逻辑**

[remux.c](../ff_remux/remux.c)

[remux.html](../server/remux.html)
