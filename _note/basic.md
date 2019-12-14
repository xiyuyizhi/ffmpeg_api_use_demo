# 基本解封装、解码流程

> 首先需要编译 ffmpeg 源码得到 avformat、avcodec、avutil、swscale 这几个静态库,ffmpeg configure 时可指定不需要的一些功能来减小静态库的体积,[见](./compile.md)

avformat 主要提供文件解封装、io 操作相关功能 比如解封装 ts、mp4,可读取本地文件,远程文件 url 地址,`自定义io操作`,[见](./avio_context.md)等

avcodec 主要提供解码相关功能,比如解码 h264、hevc

swscale 主要提供视频帧颜色转换、大小缩放等功能,比如 视频 yuv 转 rgba

avutil 提供一些公用的函数

## 解封装

> ffmpeg 相关功能都是在`上下文`中进行的,对解封转有 AvFormatContext,对自定义 IO 操作有 AvIOContext,对解码有 AvCodecContext 等。

### AVFormatContext

> 解封转后,avformat 上下文中就存储了和视频相关的一些重要信息,一共有几个原始流? 视频原始流所使用的的编码器的信息? 视频的宽高? 音频的 channel、采样率等等

解封装首先需要自己分配 AvFormatContext

```c
AvFormatContext *fmtCtx = avformat_alloc_context();
```

或者由 avformat_open_file()方法自动分配,自己只需提供一个上下文的指针

```c

AvFormatContext *fmtCtx = avformat_alloc_context();
avformat_open_file(&fmtCtx,url,NULL,NULL);

```

#### 最基本的打开文件,dump 文件信息代码:

```c

AvFormatContext *fmtCtx = NULL;
avFormat_open_file(&fmtCtx,argv[1],NULL,NULL);
av_dump_format(fmtCtx)
avformat_close_input(&fmtCtx);

```

avformat_open_file() 打开文件后,就可以探测视频的一些信息(format 上下文上的具体属性),比较重要的属性有:

```c
fmtCtx->nb_streams // 原始流数量
fmtCtx->streams // 原始流列表
fmtCtx->iformat // 输入文件的封装格式信息,对不同的封装格式有不同的具体AVInputFormat与之对应
fmtCtx->pb // 用于自定义IO时
```

解封装后 通过原始流的编码器信息,自己分配对应的解码器上下文来接着进行音视频解码,这就需要重点关注`fmtCtx->streams`,fmtCtx->streams 中是`AVStream`结构的列表

### AVStream

> 原始流结构

比较重要的属性:

```c
AVStream *stream = fmtCtx->streams[0];
stream->index // 原始流在封装格式文件中的索引下标

stream->time_base // 时间单位信息，十分重要, 所有和时间相关的数据都是基于此单位的,比如对ts文件,时间单位一般都是90kHZ,即 1s = 90000,所有 如果m3u8中第一个 ts 分片的start_time=7200,则第一帧在timeline中时间就是 7200 / 90000 = 0.08,在0.08s时间点展示播放

// 对mp4格式,时间单位一般是12800

stream->start_time // 帧，采样的开始时间 十分重要
stream->duration // 由start_time 和 duration 在结合stream->time_base 可以得到文件在timeline中的时间范围 [0s,10s] [10s,20s]等

stream->codecpar //编码器相关信息,也是十分重要 老版本ffmpeg是通过stream->codec->xxx,新版本有所改动,codecpar 是`AVCodecParameters` 结构的数据
```

### AVCodecParameters

> 编解码器相关信息,通过`stream->codecpar`得到编码器的信息,借此来分配解码器进行解码

重要属性有:

```c
AVStream *stream = fmtCtx->streams[0];
AVCodecParameters * codecParams = stream->codecpar

codecParams->codec_type // 编码的数据类型,视频?(AVMEDIA_TYPE_VIDEO) 音频?(AVMEDIA_TYPE_AUDIO) 字幕?(AVMEDIA_TYPE_SUBTITLE)

codecParams->codec_id //编码器的一个唯一枚举标记,音频aac?(AV_CODEC_ID_H264) 视频h264?(AV_CODEC_ID_H264) 视频hevc?(AV_CODEC_ID_HEVC)

codecParams->profile //编码器采用的能力级别, 比如 h264编码器 使用 Baseline Profile、Main Profile、High Profile 等

codecParams->level // 编码器对视频本身特性的描述,码率范围、分辨率范围等

codecParams->extradata // 初始化解码器需要的一些额外信息,十分重要,z在后序初始化解码器上线文是需要通过avcodec_parameters_to_context() 来将编码器的一些信息填充给解码器上下文,这里extradata就是需要的

codecParams->width

codecParams->height

codecParams->channels

codecParams->sample_rate

```

### AVCodecContext

> 解码器上下文,初始化后就可进行音视频解码

流程:

1. 分配解码器
2. 分配解码器上下文
3. 编码器信息填充至解码器上下文
4. 打开解码器

以打开视频解码器为例,忽略异常处理,资源回收:

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
avcodec_open2(decodeCtx, decoder, NULL);

```

[解码流程](./decode.md)
