# flv 直播流解析

> 直播流是一个不断拉取、解码的过程,主要有一点,新获取的直播流 buffer 能够传递给 ffmpeg `保持内存可控` 且`正常解码`

## 保持内存可控

`保持内存可控需要能够重用内存空间`,这里定义 循环 buffer 结构体

```c
#define CYCLE_BUFFER_SIZE 1024 * 1024 * 3

typedef struct _CycleBuffer
{
  uint8_t *ptr; // aviocontext read_packet回调 下一次读取视频buffer的位置
  uint8_t *ptr_end; // 结构体中可用视频buffer的结束位置
  uint8_t *ori_ptr; // 循环buffer的原始开始位置
  uint8_t *ori_ptr_end; // 循环buffer的原始结束位置  = ptr + CYCLE_BUFFER_SIZE
  size_t size; // 剩余可用视频buffer的size
} CycleBuffer;


使用流程:

static CycleBuffer cycleBuffer

//初始化(初次接受js层面传递的直播流数据时)
init(uint8_t *buffer,size_t buffer_size){
  if (!cycleBuffer.ptr)
  {
    cycleBuffer.ptr_end = cycleBuffer.ptr = cycleBuffer.ori_ptr = malloc(CYCLE_BUFFER_SIZE);
    cycleBuffer.ori_ptr_end = cycleBuffer.ori_ptr + CYCLE_BUFFER_SIZE;
    cycleBuffer.size = 0;
  }
  memcpy(cycleBuffer.ptr, buffer, buffer_size);
  cycleBuffer.ptr_end = cycleBuffer.ptr + buffer_size;
  cycleBuffer.size = buffer_size;
}

//后续新增直播流时
do_append_buffer(uint8_t *buffer, size_t buffer_size){
  int rest_to_ori_end = cycleBuffer.ori_ptr_end - cycleBuffer.ptr_end;

  printf("append..... buffer_size:%d ,rest_to_ori_end:%d \n", buffer_size, rest_to_ori_end);

  // 新增buffer大小超过循环buffer的end,部分数据要开始从循环buffer头部开始添加
  if (rest_to_ori_end > buffer_size)
  {
    memcpy(cycleBuffer.ptr_end, buffer, buffer_size);
    cycleBuffer.ptr_end += buffer_size;
  }
  else if (rest_to_ori_end == buffer_size)
  {
    memcpy(cycleBuffer.ptr_end, buffer, buffer_size);
    cycleBuffer.ptr_end = cycleBuffer.ori_ptr;
  }
  else
  {
    int rest = buffer_size - rest_to_ori_end;
    memcpy(cycleBuffer.ptr_end, buffer, rest_to_ori_end);
    memcpy(cycleBuffer.ori_ptr, buffer + rest_to_ori_end, rest);
    cycleBuffer.ptr_end = cycleBuffer.ori_ptr + rest;
  }

  cycleBuffer.size += buffer_size;
  printf("do append buffer......, add:%d , rest to custom:%d , %d, %d \n",
         buffer_size,
         cycleBuffer.size,
         cycleBuffer.ptr_end - cycleBuffer.ptr,
         cycleBuffer.ori_ptr_end - cycleBuffer.ptr);
}


// aviocontext 的read_packet回调中也要考虑从循环buffer中要数据时到达了循环Buffer结束位置，然后从头开始
static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
  CycleBuffer *bd = (CycleBuffer *)opaque;

  buf_size = MIN(bd->size, buf_size);

  if (!buf_size)
  {
    printf("no buf_size pass to read_packet,%d , %d \n", buf_size, bd->size);
    return -1;
  }

  int rest_to_ori_end = bd->ori_ptr_end - bd->ptr;

  // printf("bd->size: %d , %d ,rest_to_ori_end:%d \n", bd->size, buf_size, rest_to_ori_end);

  if (rest_to_ori_end >= buf_size)
  {
    memcpy(buf, bd->ptr, buf_size);
    bd->ptr += buf_size;
  }
  else
  {
    // 到达结束位置
    printf("----------reach end %d \n", rest_to_ori_end);
    memcpy(buf, bd->ptr, rest_to_ori_end);
    memcpy(buf + rest_to_ori_end, bd->ori_ptr, buf_size - rest_to_ori_end);
    bd->ptr = bd->ori_ptr + buf_size - rest_to_ori_end;
  }

  bd->size -= buf_size;
  return buf_size;
}


```

## 新增 buffer 能够正常解码

解码过程遇到的问题有:

直播流数据的首次处理(这里拉取 2、3 百 KB 数据后开始 ffmpeg 处理),avFormatCtx 会一次把所有数据读入内部内存(探测视频信息，解封转处理),导致循环执行 av_read_packet()时最后返回 EOF(最后的一些 buffer 不够完整的一帧就结束了),`导致后续新增buffer后继续av_read_frame()时 报 missmacth packet错误`

解决方法:

1. 指定 probesize

   avFormatContext 结构体有个 probesize 属性，指定初始进行视频探测时允许探测的最大 buffer 长度

   `指定较小的值 防止初始视频buffer被全部读入`

   ```c
   #define PROBE_BUFFER_SIZE 4096 * 30

   gState.fmtCtx.probesize = PROBE_BUFFER_SIZE;

   ```

2. av_read_frame() 循环执行的结束条件控制

   当 cycleBuffer.size < avioCtx 的内部 buffer 长度时停止,防止读到新增 buffer 的结束位置

   ```c

   int do_read_pkts(){
     int result = 0;
     while (result == 0 && (cycleBuffer.size > IO_CTX_BUFFER_SIZE))
     {
       AVPacket *pkt = av_packet_alloc();
       av_init_packet(pkt);
       result = av_read_frame(gState.fmtCtx, pkt);
       if (result == 0)
       {
         // do something
       }
       else
       {
         av_packet_unref(pkt);
         av_packet_free(pkt);
       }
     }
   }

   ```
