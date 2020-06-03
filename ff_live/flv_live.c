
#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"

#define MAIN_RUN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define IO_CTX_BUFFER_SIZE 4096 * 4
#define CYCLE_BUFFER_SIZE 1024 * 800
#define PROBE_BUFFER_SIZE 4096 * 25
#define MAX_PKT_QUEUE_SIZE 512

typedef void (*VideoCallback)(uint8_t *ybuffer, uint8_t *ubuffer, uint8_t *vbuffer, int64_t pts);
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
  AVCodec *codecCtx;
  AVStream *vStream;
  AVPacket *videoPacketList[MAX_PKT_QUEUE_SIZE];
  int64_t videoStartPTS;
  int64_t audioStartPTS;
  int vStreamIndex;
  int width;
  int height;
} GState;

enum Error_Code
{
  OPEN_FILE_ERROR = -1,
  FIND_STREAM_INFO_ERROR = -2,
  FIND_BEST_STREAM_ERROR = -3,
  OPEN_DECODE_CONTEXT_ERROR = -4,
  AVFRAME_ALLOC_ERROR = -5,
  SWSCTX_ALLOC_ERROR = -6,
  AVIO_ALLOC_ERROR = -7
};

static GState gState;
static CycleBuffer cycleBuffer;

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

  printf("bd->size: %d , %d ,rest_to_ori_end:%d \n", bd->size, buf_size, rest_to_ori_end);

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

  gState->v_meta_callback(params->width,
                          params->height,
                          stream->time_base.den,
                          stream->start_time);
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
  int i = 0;
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
      }
    }
    else
    {
      av_packet_unref(pkt);
      av_packet_free(pkt);
    }
  }

  for (int j = 0; j < i; j++)
  {
    AVPacket *pkt = gState.videoPacketList[j];
    av_packet_unref(pkt);
    av_packet_free(pkt);
  }

  printf("do_read_pkts: video pkt:%d  result:%d , buffer rest to custom:%d \n", i, result, cycleBuffer.size);
}

enum Error_Code decodeVideoFrame()
{
}

int init_process(long v_meta_cb, long v_frame_cb)
{
  gState.v_meta_callback = v_meta_cb;
  gState.v_callback = v_frame_cb;
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
}

#ifdef MAIN_RUN

void *print_video_metadata(int width, int height, int timescale, int64_t start_time)
{
  printf("video metadata : width=%d , height=%d, timescale=%d , start_time:%lld \n", width, height, timescale, start_time);
}

void *write_frame_to_file(uint8_t *buffer, size_t size) {}

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

  int size1 = 300 * 1024;
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

  do_demuxer(buffer, buffer_size);
  do_read_pkts(0);

  // do_demuxer(split1, size1);
  // do_read_pkts(0);

  // do_append_buffer(split2, size2);
  // do_read_pkts(0);

  // do_append_buffer(split3, size3);
  // do_read_pkts(0);

  // do_append_buffer(split4, size4);
  // do_read_pkts(0);

  av_file_unmap(buffer, buffer_size);
  free(split1);
  free(split2);
  free(split3);
  free(split4);
  free(cycleBuffer.ori_ptr);
  clean();
}
#endif