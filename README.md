# ffmpeg_api_use_demo

# FFmpeg API 梳理

## avFormat

> IO 和 muxing demuxing lib

file -> demux -> decode -> es stream

es stream -> encode ->mux -> file

### 重要结构体

#### AVFormatContext

> exports all information about the file being read or written

使用 avformat_alloc_context() 创建, avforamt_free_context()释放

```javascript

AvFormatContext *fmt_ctx =NULL;
// fmt_ctx = avformat_alloc_context()
avformat_open_file(&fmt_ctx,url,NULL,NULL); // 使用avformat_close_input() close

av_find_best_stream(&fmt_ctx,AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0) // find 指定流

```

AvFormatContext 中重要属性:

fmt_ctx->iformat // AVInputFormat

fmt_ctx->oformat // AVOutputFormat

fmt_ctx->streams // AVStream 原始流列表

fmt_ctx->nb_frames // 原始流数量

fmt_ctx->pb // io context

#### AVStream

> 代表流结构的定义

对于解封装,streams are created by libavformat in avformat_open_input().

可使用 avformat_new_stream() 创建新流

```javascript

  for (int i = 0; i < fmt_ctx->nb_streams; i++)
    {
        AVStream *s = fmt_ctx->streams[i];
        prinf("codec_type=%d,codec_id=%d\n",s->codec_type,s->codec_id);
    }

```

AVStream 中重要属性:

stream->index // 此流在 AvFormatContext 中流列表的索引

stream->id // 原始流的 id

stream->metadta // 元数据信息 AVDictionary 结构

stream->codecpar // 与流相关的编解码器参数信息,据此信息 找到 decoder,并创建 AVCodecContext!!

#### AVCodecParameters

> describes the properties of an encoded stream

可通过 avcodec_parameters_alloc() 创建,通过 avcodec_parameters_free()释放资源

```javascript

    AVStream *s = fmt_ctx->streams[0]; // 获取原始流1
    AVCodecParameters *params = s->codecpar
    printf("codec_type=%d,codec_id=%d,width=%d,height=%d\n",params->codec_type,params->codec_id,params->width,params->height);
    av_get_media_type_string(params->codec_type) //获取 codec_type对象的字符串表示

    //根据编码器id(codec_id) 找到对应的解码器
    AVCodec *dec =  avcodec_find_decoder(params->codec_id);;
    printf("codec name=%s,long_name=%s\n", dec->name, dec->long_name);
    AVCodecContext *dec_ctx = avcodec_alloc_context3(dec);
```

AVCodecParameters 中重要属性:

params->codec_type // 指定具体数据的类型,视频?音频？字幕？ 定义在枚举 AVMediaType 中常用:

```
AVMEDIA_TYPE_VIDEO
AVMEDIA_TYPE_AUDIO
AVMEDIA_TYPE_SUBTITLE

```

params->codec_id // 编码数据所使用的编解码器 h264？ aac?

```
AV_CODEC_ID_H264 27
AV_CODEC_ID_AAC  86018
AV_CODEC_ID_HEVC 173
```

params->profile

params->level

params->bit_rate // 平均码率

params->width //video only

params->height //video only

params->channels //audio only

params->frame_size //audio only

params->sample_rate //audio only

#### AVDictionary

> av 字典类型,存储键值对

遍历所有字段

```javascript

// AVStream s
AVDictionaryEntry *prev = NULL;
printf("    metadata:count=%d\n", av_dict_count(s->metadata));
while ((prev = av_dict_get(s->metadata, "", prev, AV_DICT_IGNORE_SUFFIX)) != NULL)
{
    printf("      key=%s,value=%s\n", prev->key, prev->value);
}
```

#### AVPacket

> 用来存储压缩后的数据,可以有两个出处：

1. 容器格式文件数据被解封装后导出的 AVPacket 结构数据

2. 原始流数据经过编码器编码后得到的 AVPacket 结构数据,然后经 mux 封装到容器格式

对于视频,AVPacket 结构中就包含一帧压缩的视频数据,对于音频一般包含几帧数据。

```javascript
av_read_frame() // 迭代读取帧数据

AVPacket pack;
av_init_packet(&pack);
while (av_read_frame(fmt_ctx, &pack) == 0)
{
  // do something
  av_packet_unref(&pack);
  av_init_packet(&pack);
}

```

重要属性:

pack.pts

pack.dts

pack.size //数据 size

pack.stream_index //代表存储的数据是哪种原始流的

pack.duration

#### AVCodec

#### AVCodecContext

->pix_fmt

#### AVFrame

#### AVPixelFormat

#### AVIOContext

#### SwsContext

### workflow

1. open file

2. store format info in avformatcontext

3. find streams from formatcontext

4. find codec info from stream.codepar

5. find matched decoder

6. alloc context for decoder

7. init codec‘s context

8. iterative read compressed data from formatcontext. stored in avpacket

9. decode frame data use codeccontext and store decompressed data in avframe

10. get decompressed data from avframe and store it in buffer or any other operate
