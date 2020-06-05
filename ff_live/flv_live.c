
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"

// #define MAIN_RUN
#define FRAME_SAVE_AS_RGB
// #define FRAME_SAVE_AS_YUV
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define IO_CTX_BUFFER_SIZE 4096 * 4
#define CYCLE_BUFFER_SIZE 1024 * 1024 * 3
#define PROBE_BUFFER_SIZE 4096 * 30
#define MAX_PKT_QUEUE_SIZE 1024

typedef void (*VideoCallback)(uint8_t *ybuffer, uint8_t *ubuffer, uint8_t *vbuffer, int width, int height, int size, int64_t pts);
typedef void (*VideoMetaDataCallback)(int width, int height, int timescale, int64_t start_time);

typedef struct _CycleBuffer
{
  uint8_t *ptr;
  uint8_t *ptr_end;
  uint8_t *ori_ptr;
  uint8_t *ori_ptr_end;
  size_t size;
} CycleBuffer;

typedef struct _GState
{
  VideoCallback v_callback;
  VideoMetaDataCallback v_meta_callback;
  AVFormatContext *fmtCtx;
  AVCodecContext *codecCtx;
  AVStream *vStream;
  AVFrame *frame;
  AVPacket *videoPacketList[MAX_PKT_QUEUE_SIZE];
  int pktAppendIndex;
  int pktDecodeIndex;
  int64_t videoStartPTS;
  int64_t audioStartPTS;
  int vStreamIndex;
  int width;
  int height;
  struct SwsContext *swsCtx;
#ifdef FRAME_SAVE_AS_RGB
  uint8_t *dst_data[4];
  int dst_linesize[4];
  int rgbSize;
#endif
} GState;

#ifdef FRAME_SAVE_AS_YUV
typedef struct _YUV
{
  int width;
  int height;
  uint8_t *ybf;
  uint8_t *ubf;
  uint8_t *vbf;
} YUV;
#endif

enum Error_Code
{
  OPEN_FILE_ERROR = -1,
  FIND_STREAM_INFO_ERROR = -2,
  FIND_BEST_STREAM_ERROR = -3,
  OPEN_DECODE_CONTEXT_ERROR = -4,
  AVFRAME_ALLOC_ERROR = -5,
  SWSCTX_ALLOC_ERROR = -6,
  AVIO_ALLOC_ERROR = -7,
  YUV_BUFFER_ALLOC_ERROR = -8
};

static GState gState;
static CycleBuffer cycleBuffer;
#ifdef FRAME_SAVE_AS_YUV
static YUV yuv;
#endif

av_image_alloc();

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
    printf("----------reach end %d \n", rest_to_ori_end);
    memcpy(buf, bd->ptr, rest_to_ori_end);
    memcpy(buf + rest_to_ori_end, bd->ori_ptr, buf_size - rest_to_ori_end);
    bd->ptr = bd->ori_ptr + buf_size - rest_to_ori_end;
  }

  bd->size -= buf_size;
  return buf_size;
}

enum Error_Code init_custom_io(CycleBuffer *bd, GState *gState)
{
  AVIOContext *avioCtx;
  uint8_t *ioCtxBuffer = av_malloc(IO_CTX_BUFFER_SIZE);

  avioCtx = avio_alloc_context(ioCtxBuffer, IO_CTX_BUFFER_SIZE, 0, bd, &read_packet, NULL, NULL);

  if (!avioCtx)
  {
    return AVIO_ALLOC_ERROR;
  }

  gState->fmtCtx->pb = avioCtx;
  gState->fmtCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
  return 0;
}

enum Error_Code init_video_decoder(GState *gState)
{
  AVCodec *codec;
  int result = av_find_best_stream(gState->fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &codec, 0);
  if (result < 0)
  {
    return FIND_BEST_STREAM_ERROR;
  }
  gState->vStreamIndex = result;
  AVStream *stream = gState->fmtCtx->streams[result];
  AVCodecParameters *params = stream->codecpar;
  gState->vStream = stream;
  gState->codecCtx = avcodec_alloc_context3(codec);

  result = avcodec_parameters_to_context(gState->codecCtx, params);
  if (result < 0)
  {
    return OPEN_DECODE_CONTEXT_ERROR;
  }
  result = avcodec_open2(gState->codecCtx, codec, NULL);
  if (result < 0)
  {
    return OPEN_DECODE_CONTEXT_ERROR;
  }

  gState->width = params->width;
  gState->height = params->height;
  gState->v_meta_callback(params->width,
                          params->height,
                          stream->time_base.den,
                          stream->start_time);
  return 0;
}

enum Error_Code do_demuxer(uint8_t *buffer, size_t buffer_size)
{
  int result = 0;
  printf("do demuxer size:%d \n", buffer_size);

  if (!cycleBuffer.ptr)
  {

    cycleBuffer.ptr_end = cycleBuffer.ptr = cycleBuffer.ori_ptr = malloc(CYCLE_BUFFER_SIZE);
    cycleBuffer.ori_ptr_end = cycleBuffer.ori_ptr + CYCLE_BUFFER_SIZE;
    cycleBuffer.size = 0;
  }

  memcpy(cycleBuffer.ptr, buffer, buffer_size);
  cycleBuffer.ptr_end = cycleBuffer.ptr + buffer_size;
  cycleBuffer.size = buffer_size;

  gState.fmtCtx = avformat_alloc_context();

  //set probesize
  gState.fmtCtx->probesize = PROBE_BUFFER_SIZE;
  result = init_custom_io(&cycleBuffer, &gState);
  if (result < 0)
  {
    return result;
  }

  result = avformat_open_input(&gState.fmtCtx, "", NULL, NULL);
  if (result < 0)
  {
    return OPEN_FILE_ERROR;
  }

  result = avformat_find_stream_info(gState.fmtCtx, NULL);
  if (result < 0)
  {
    return FIND_BEST_STREAM_ERROR;
  }

  result = init_video_decoder(&gState);
  if (result < 0)
  {
    return result;
  }

#ifdef FRAME_SAVE_AS_YUV
  if (yuv.width != gState.width || yuv.height != gState.height)
  {
    yuv.width = gState.width;
    yuv.height = gState.height;
    if (yuv.ybf)
    {
      free(yuv.ybf);
      free(yuv.ubf);
      free(yuv.vbf);
    }
    yuv.ybf = malloc(yuv.width * yuv.height);
    yuv.ubf = malloc(yuv.width / 2 * yuv.height / 2);
    yuv.vbf = malloc(yuv.width / 2 * yuv.height / 2);
    if (yuv.ybf == NULL || yuv.ubf == NULL || yuv.vbf == NULL)
    {
      return YUV_BUFFER_ALLOC_ERROR;
    }
  }
#endif

#ifdef FRAME_SAVE_AS_RGB
  gState.swsCtx = sws_getContext(
      gState.width,
      gState.height,
      gState.codecCtx->pix_fmt,
      gState.width,
      gState.height,
      AV_PIX_FMT_RGBA,
      NULL,
      NULL,
      NULL,
      NULL);
  if (!gState.swsCtx)
  {
    return SWSCTX_ALLOC_ERROR;
  };
  gState.rgbSize = av_image_alloc(gState.dst_data, gState.dst_linesize, gState.width, gState.height, AV_PIX_FMT_RGBA, 1);
#endif

  return 0;
}

int do_append_buffer(uint8_t *buffer, size_t buffer_size)
{

  int rest_to_ori_end = cycleBuffer.ori_ptr_end - cycleBuffer.ptr_end;

  printf("append..... buffer_size:%d ,rest_to_ori_end:%d \n", buffer_size, rest_to_ori_end);

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

int do_read_pkts(toEnd)
{

  int result = 0;
  int i = gState.pktAppendIndex;
  while (result == 0 && (cycleBuffer.size > IO_CTX_BUFFER_SIZE || toEnd))
  {
    AVPacket *pkt = av_packet_alloc();
    av_init_packet(pkt);
    result = av_read_frame(gState.fmtCtx, pkt);

    if (result == 0)
    {
      if (pkt->stream_index == gState.vStreamIndex)
      {
        gState.videoPacketList[i++] = pkt;
        if (i >= MAX_PKT_QUEUE_SIZE)
        {
          printf("max queue pkts length reach,reset to 0 \n");
          i = 0;
        }
      }
    }
    else
    {
      av_packet_unref(pkt);
      av_packet_free(pkt);
    }
  }

  gState.pktAppendIndex = i;

  printf("do_read_pkts: video pkt:%d  result:%d , buffer rest to custom:%d \n", i, result, cycleBuffer.size);
}

int resolve_video_frame(AVFrame *frame)
{
  int result = avcodec_receive_frame(gState.codecCtx, frame);
  int width;
  int height;
  char *y;
  char *u;
  char *v;

  if (result != 0)
  {
    printf("Error: avcodec recieve frame,%d\n", result);
    return result;
  }
  width = frame->width;
  height = frame->height;

#ifdef FRAME_SAVE_AS_YUV
  y = frame->data[0];
  u = frame->data[1];
  v = frame->data[2];
  if (frame->linesize[0] != width)
  {
    for (int i = 0; i < height; i++)
    {
      memcpy(yuv.ybf + width * i, frame->data[0] + frame->linesize[0] * i, width);
    }
    y = yuv.ybf;
  }
  if (frame->linesize[1] != width / 2)
  {
    for (int i = 0; i < height / 2; i++)
    {
      memcpy(yuv.ubf + width / 2 * i, frame->data[1] + frame->linesize[1] * i, width / 2);
      memcpy(yuv.vbf + width / 2 * i, frame->data[1] + frame->linesize[1] * i, width / 2);
    }
    u = yuv.ubf;
    v = yuv.vbf;
  }
#endif

#ifdef FRAME_SAVE_AS_RGB
  sws_scale(gState.swsCtx, frame->data, frame->linesize, 0, frame->height, gState.dst_data, gState.dst_linesize);
  gState.v_callback(gState.dst_data[0], u, v, width, height, gState.rgbSize, frame->pts);
#endif

#ifdef FRAME_SAVE_AS_YUV
  gState.v_callback(y, u, v, width, height, width * height, frame->pts);
#endif

  return 0;
}

enum Error_Code decodeVideoFrame()
{
  int decode_count = 0;
  int result = 0;
  int i = gState.pktDecodeIndex;
  int order = gState.pktAppendIndex > gState.pktDecodeIndex;
  int maxDecodeLen = order ? gState.pktAppendIndex - gState.pktDecodeIndex : MAX_PKT_QUEUE_SIZE - gState.pktDecodeIndex + gState.pktAppendIndex;
  AVFrame *frame = gState.frame;
  AVPacket *pkt = gState.videoPacketList[i];

  printf("max decode length:%d \n", maxDecodeLen);
  while (pkt && decode_count < maxDecodeLen)
  {
    result = avcodec_send_packet(gState.codecCtx, pkt);
    if (result != 0)
    {
      printf("Error: avcodec_send_packet ,%d , i=%d \n", result, i);
    }
    else if (resolve_video_frame(frame) >= 0)
    {
      decode_count++;
    }
    av_packet_unref(pkt);
    av_packet_free(&pkt);
    gState.videoPacketList[i++] = NULL;
    if (i >= MAX_PKT_QUEUE_SIZE)
    {
      i = 0;
    }
    pkt = gState.videoPacketList[i];
  }

  gState.pktDecodeIndex = i;
  printf("video decode finish: %d \n", decode_count);
}

int init_process(long v_meta_cb, long v_frame_cb)
{
  gState.v_meta_callback = v_meta_cb;
  gState.v_callback = v_frame_cb;
  gState.pktAppendIndex = 0;
  gState.pktDecodeIndex = 0;
  gState.frame = av_frame_alloc();
}

clean()
{
  if (gState.fmtCtx && gState.fmtCtx->pb)
  {
    av_freep(&gState.fmtCtx->pb->buffer);
    avio_context_free(&gState.fmtCtx->pb);
  }

  if (gState.fmtCtx)
  {
    avformat_close_input(&gState.fmtCtx);
  }

  if (gState.codecCtx)
  {
    avcodec_close(gState.codecCtx);
    avcodec_free_context(&gState.codecCtx);
  }
  if (gState.frame)
  {
    av_frame_free(&gState.frame);
  }
  if (cycleBuffer.ori_ptr)
  {
    free(cycleBuffer.ori_ptr);
  }

#ifdef FRAME_SAVE_AS_RGB
  if (gState.dst_data[0])
  {
    av_freep(&gState.dst_data[0]);
  }
#endif
}

#ifdef MAIN_RUN
void *print_video_metadata(int width, int height, int timescale, int64_t start_time)
{
  printf("video metadata : width=%d , height=%d, timescale=%d , start_time:%lld \n", width, height, timescale, start_time);
}

char *concat_strarray(char **s, int length)
{
  int new_len = 0;
  for (int i = 0; i < length; i++)
  {
    new_len += strlen(s[i]);
  }
  char *n_s = malloc(new_len + 1);
  *n_s = '\0';

  for (int i = 0; i < length; i++)
  {
    strcat(n_s, s[i]);
  }
  return n_s;
}

void *write_frame_to_file(uint8_t *s, uint8_t *ubuffer, uint8_t *vbuffer, int width, int height, int size, int64_t pts)
{
  char s_index[20];
  sprintf(s_index, "%lld", pts);
  char *carray[3] = {
      "./_temp/frame",
      s_index,
      ".rgb"};
  char *file_name = concat_strarray(carray, 3);
  FILE *file = fopen(file_name, "wb");
  fwrite(s, 1, size, file);
  fclose(file);
}

int main(int argc, char **argv)
{

  if (argc < 2)
  {
    printf("./demo filename \n");
    return 0;
  }

  char *file_name = argv[1];
  uint8_t *buffer;
  size_t buffer_size;
  av_file_map(file_name, &buffer, &buffer_size, 0, NULL);

  printf("input buffer size:%d \n", buffer_size);

  int size1 = 200 * 1024;
  int size2 = 150 * 1024;
  int size3 = 50 * 1024;
  int size4 = 200 * 1024;

  uint8_t *split1 = malloc(size1);
  uint8_t *split2 = malloc(size2);
  uint8_t *split3 = malloc(size3);
  uint8_t *split4 = malloc(size4);

  memcpy(split1, buffer, size1);
  memcpy(split2, buffer + size1, size2);
  memcpy(split3, buffer + size1 + size2, size3);
  memcpy(split4, buffer + size1 + size2 + size3, size4);

  init_process(print_video_metadata, write_frame_to_file);

  do_demuxer(split1, size1);
  do_read_pkts(0);
  decodeVideoFrame();

  do_append_buffer(split2, size2);
  do_read_pkts(0);
  decodeVideoFrame();

  do_append_buffer(split3, size3);
  do_read_pkts(0);
  decodeVideoFrame();

  do_append_buffer(split4, size4);
  do_read_pkts(0);
  decodeVideoFrame();

  av_file_unmap(buffer, buffer_size);
  free(split1);
  free(split2);
  free(split3);
  free(split4);
  free(cycleBuffer.ori_ptr);
  cycleBuffer.ori_ptr = NULL;
  clean();
}
#endif