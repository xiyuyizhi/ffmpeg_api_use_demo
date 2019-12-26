# 关于提取音频 pcm 数据、视频 yuv 数据

> 解封转、初始化解码器后 解码数据,主要借助几个结构体,AVFrame AVPacket,SwsContext

AVPacket 存储编码的数据,对视频,AVPacket 存储一帧数据,对音频 可以存储多帧(没帧又包含多个采样)

AVFrame 存储解码后的数据

SwsContext 视频帧操作上下文

### 解码视频

首先 分配 AVPacket、AVFrame

```c
AVPacket *pkt =NULL;
AVFrame *frame =NULL;
av_init_packet(pkt);
frame = av_frame_alloc()
```

AVPacket 主要属性:

```
pkt->stream_index // 区别帧类型,视频帧？ 音频帧?
pkt->data // 存放编码的帧数据
pkt->pts // 帧的展示时间
pkt->dts // 帧的解码时间
pkt->buf // AVBufferRef 结构,指向用来存储pkt->data的引用计数的buffer,当数据不在使用时要通过av_packet_unref() 来减少引用
pkt->size // 剩余的pkt->data的buffer长度,在解码音频时常用,因为packet包中可能包含多帧音频

```

AVFrame 主要属性:

```c

frame->format //解压后的帧数据格式,对视频是枚举类型【AVPixelFormat】中定义的,例如 AV_PIX_FMT_YUV420P, 对音频是枚举类型【AVSampleFormat】中定义的,例如AV_SAMPLE_FMT_FLTP (每个采样采用32位float存储,且不同channel存储在不同平面)

frame->data // 十分重要,是指针数组类型,每个指针指向一个平面

frame->linesize // 十分重要 int数据类型,代表每个平面中每行数据大小

frame->pict_type // 帧类型,对视频来说是 I帧? P帧? B帧？

frame->pts // 帧展示时间 十分重要

frame->sample_rate // 对音频

frame->channels // 对音频

frame->width // 对视频

frame->height //对视频

```

#### 关于平面

> 采样数据的存储方式,与具体编码方式相关

例如对视频 每一帧采用 YUV420P 表示,那么解压后 YUV 数据分别存储在 frame->data[0] frame->data[1] frame->data[2] , YUV 数据每一行的长度存储在 frame->linesize[0]、frame->linesize[1] frame->linesize[2]中

例如对音频 每一帧包含两个 channel,那么 每个 channel 的数据存储在单独的平面,左信道数据在 frame->data[0]，右信道数据在 frame->data[1]

#### 视频解码 API

获取一帧压缩的数据

```
av_read_frame(fmtCtx,pkt)
```

解码视频用到的两个方法:

avcodec_send_packet(decodeCtx,pkt) //将一帧压缩的数据传递给解码器上下文

avcodec_receive_frame(decodeCtx,frame) // 从解码器上下文获取一帧解码后的数据

avcodec_receive_frame 方法返回值:

0: success

AVERROR(EAGAIN): output is not available in this state - user must try to send new input

AVERROR_EOF: the decoder has been fully flushed, and there will be no more output frames

AVERROR(EINVAL): codec not opened, or it is an encoder

AVERROR_INPUT_CHANGED: current decoded frame has changed parameters with respect to first decoded frame

**Note:**

解码器内部是会缓存视频帧的,所以在循环读取 packet、解码帧、读取解码后的帧数据 直到 av_read_frame() < 0(表示视频文件中没有新的压缩帧数据了)后,解码器中含缓存了几帧数据,还要读出来!!

伪代码:

```c

int  endOfFile = 0;
int result;
do {

  if (av_read_frame(fmtCtx, pkt) < 0)
  {
    endOfFile = 1;
  }
  if(pkt->stream_index==视频帧){
     result = avcodec_send_packet(decodeCtx, pkt);
     if(result==0){
       if (avcodec_receive_frame(decodeCtx, frame) >= 0)
        {
          // 处理解码后的数据
        }
     }
     if(endOfFile){
       // 读出最后几帧
        while(result ==0){
            result = avcodec_receive_frame(decodeCtx, frame);
        }
     }
  }

}while(!endOfFile)

```

#### update

视频解码获取到YUV数据后,前端可以直接通过webgl渲染,Y、U、V 三平面的linesize 不一定和 width、width/2、width/2相同(ffmpeg会做一些对齐操作,插入一些字节用来优化),测试发现对高清码率linesize和width一般不对应

对于不对应的情形,需要把实际像素值提取出来
``` 

char *ybuffer = malloc(frame->width,frame->height);
char *ubuffer = malloc(frame->width * frame->height / 4);
char *vbuffer = malloc(frame->width * frame->height / 4);
int width = frame->width;
int height = frame->height;

if(frame->lineszie[0] != width){
  for( int i=0;i<height;i++){
     memcpy(ybuffer + width * i, frame->data[0] + frame->linesize[0] * i, width);
  }
}

if(frame->linesize[1] != width /2){
  for(int i=0;i<height /2 ;i++){
      memcpy(ubuffer + width /2 * i, frame->data[1] + frame->linesize[1] * i, width /2);
      memcpy(vuffer + width /2* i, frame->data[2] + frame->linesize[2] * i, width/2);
  }
}

// free yuv buffer when no use
free(ybuffer);
free(ubuffer);
free(vbuffer);

```


### 解码音频

> aac 编码音频数据默认解码为 AV_SAMPLE_FMT_FLTP 格式,每个 channel 数据各占一个平面

音频解码通过 avcodec_decode_audio4()

需要注意的是: 一个 avpacket 中可能存在多个音频采样,需要对 avpacket 进行循环读取,直到 pkt->size <=0

伪代码:

```c

do
  {
    result = avcodec_decode_audio4(decodeCtx, frame, &got_frame, pkt);
    if (result < 0)
    {
      printf("Error deocde audio frame\n");
      return result;
    }

    result = MIN(result, pkt->size);

    if (got_frame)
    {
        // 处理音频数据
    }
    pkt->data += result;
    pkt->size -= result;
  } while (pkt->size > 0);

```

#### 音频解码后 pcm 数据播放

音频不同声道数据存在 frame->data[0] frame->data[1]中,需转换成`LRLRLRLRLR...`格式后 可通过 `ffplay -f f32le -ac 2 1.pcm`进行播放

```c
uint8_t *translate_pcm(int data_size, AVFrame *frame, int sample_size)
{

  uint8_t *bf = av_malloc(data_size);
  int offset = 0;
  for (int i = 0; i < frame->nb_samples; i++)
  {
    for (int index = 0; index < frame->channels; index++)
    {
      memcpy(bf + offset, frame->data[index] + i * sample_size, sample_size);
      offset += sample_size;
    }
  }
  return bf;
}
```






