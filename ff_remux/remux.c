
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/timestamp.h"

#define MAIN
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define IO_CTX_BUFFER_SIZE 4096 * 8

typedef struct _BufferData
{
  uint8_t *ptr;
  uint8_t *ori_ptr;
  size_t size;
  size_t file_size;
} BufferData;
typedef struct _State
{
  AVFormatContext *iFmtCtx;
  AVFormatContext *oFmtCtx;
  int videoStreamIndex;
  int audioStreamIndex;
  int nbStreams;
  int64_t videoFirstDts;
  int64_t audioFirstDts;
  BufferData iBufData;
  BufferData oBufferData;
} State;

static State state;

static int read_packet(void *opaque, uint8_t *buf, int buf_size)
{
  BufferData *bd = (BufferData *)opaque;
  buf_size = MIN(bd->size, buf_size);

  if (!buf_size)
  {
    // printf("no buf_size pass to read_packet,%d,%d\n", buf_size, bd->size);
    return -1;
  }
  // printf("%ld ,%ld ,%ld \n", bd->file_size, bd->size, buf_size);
  memcpy(buf, bd->ptr, buf_size); //从大buffer中读视频数据
  bd->ptr += buf_size;
  bd->size -= buf_size;
  return buf_size;
}

static int64_t seek_packet(void *opaque, int64_t offset, int whence)
{
  BufferData *bd = (BufferData *)opaque;
  int64_t ret = -1;

  // printf("%lld , %d \n", offset, whence);

  switch (whence)
  {
  case AVSEEK_SIZE:
    ret = bd->file_size;
    break;
  case SEEK_SET:
    bd->ptr = bd->ori_ptr + offset;
    bd->size = bd->file_size - offset;
    ret = bd->ptr;
    break;
  }
  return ret;
}

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

AVIOContext *io_alloc(BufferData *bd, int type)
{
  AVIOContext *ioCtx;
  uint8_t *ioCtxBuffer = av_malloc(IO_CTX_BUFFER_SIZE);
  ioCtx = avio_alloc_context(
      ioCtxBuffer,
      IO_CTX_BUFFER_SIZE,
      type,
      bd,
      type == 0 ? &read_packet : NULL,
      type == 1 ? &write_packet : NULL,
      &seek_packet);
  return ioCtx;
}

int init_demux_context(State *state)
{
  state->iFmtCtx = avformat_alloc_context();
  state->iFmtCtx->pb = io_alloc(&state->iBufData, 0);
  state->iFmtCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
  if (!state->iFmtCtx->pb)
  {
    return -1;
  }

  if (avformat_open_input(&state->iFmtCtx, NULL, NULL, NULL) < 0)
  {
    printf("can not open the file!\n");
    return -1;
  }

  if (avformat_find_stream_info(state->iFmtCtx, NULL) < 0)
  {
    printf("can't find stream info");
    return -1;
  }

  state->nbStreams = state->iFmtCtx->nb_streams;

  for (int i = 0; i < state->nbStreams; i++)
  {
    int codec_type = state->iFmtCtx->streams[i]->codecpar->codec_type;
    if (codec_type == AVMEDIA_TYPE_VIDEO)
    {
      state->videoFirstDts = state->iFmtCtx->streams[i]->first_dts;
      state->videoStreamIndex = i;
    }
    if (codec_type == AVMEDIA_TYPE_AUDIO)
    {
      state->audioFirstDts = state->iFmtCtx->streams[i]->first_dts;
      state->audioStreamIndex = i;
    }
  }

  av_dump_format(state->iFmtCtx, NULL, NULL, NULL);
  return 0;
}

int init_remux_context(State *state)
{
  int ret;
  avformat_alloc_output_context2(&state->oFmtCtx, NULL, "mp4", NULL);
  if (!state->oFmtCtx)
  {
    printf("can not create output context \n");
    return -1;
  }
  state->oFmtCtx->pb = io_alloc(&state->oBufferData, 1);
  state->oFmtCtx->flags |= AVFMT_FLAG_CUSTOM_IO;

  for (int i = 0; i < state->nbStreams; i++)
  {
    AVStream *outStream;
    AVStream *inStream = state->iFmtCtx->streams[i];
    AVCodecParameters *inParams = inStream->codecpar;
    outStream = avformat_new_stream(state->oFmtCtx, NULL);
    if (!outStream)
    {
      printf("Failed allocating output stream\n");
      return -1;
    }
    ret = avcodec_parameters_copy(outStream->codecpar, inParams);
    if (ret < 0)
    {
      printf(stderr, "Failed to copy codec parameters\n");
      return -1;
    }
    outStream->codecpar->codec_tag = 0;
  }
  av_dump_format(state->oFmtCtx, 0, NULL, 1);
  return 0;
}

int do_remux(State *state)
{
  AVPacket pkt;
  int64_t dts;
  int ret = avformat_write_header(state->oFmtCtx, NULL);
  if (ret < 0)
  {
    printf("Error occurred when opening output file\n");
    return -1;
  }

  while (1)
  {
    AVStream *in_stream, *out_stream;
    ret = av_read_frame(state->iFmtCtx, &pkt);
    if (ret < 0)
      break;

    in_stream = state->iFmtCtx->streams[pkt.stream_index];
    if (pkt.stream_index != state->videoStreamIndex && pkt.stream_index != state->audioStreamIndex)
    {
      av_packet_unref(&pkt);
      continue;
    }
    out_stream = state->oFmtCtx->streams[pkt.stream_index];

    // 调整输出格式帧的pts、dts
    if (pkt.stream_index == state->videoStreamIndex)
    {
      dts = state->videoFirstDts;
    }
    if (pkt.stream_index == state->audioStreamIndex)
    {
      dts = state->audioFirstDts;
    }
    /* copy packet */
    pkt.pts = av_rescale_q_rnd(pkt.pts - dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    pkt.dts = av_rescale_q_rnd(pkt.dts - dts, in_stream->time_base, out_stream->time_base, AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX);
    pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, out_stream->time_base);
    pkt.pos = -1;
    ret = av_interleaved_write_frame(state->oFmtCtx, &pkt);
    if (ret < 0)
    {
      printf("Error muxing packet\n");
      break;
    }
    av_packet_unref(&pkt);
  }
  ret = av_write_trailer(state->oFmtCtx);
  return ret;
}

void clean()
{

  if (state.iFmtCtx && state.iFmtCtx->pb)
  {
    av_free(state.iFmtCtx->pb->buffer);
    avio_context_free(&state.iFmtCtx->pb);
    avformat_close_input(&state.iFmtCtx);
  }

  if (state.oFmtCtx && state.oFmtCtx->pb)
  {
    av_free(state.oFmtCtx->pb->buffer);
    avio_context_free(&state.oFmtCtx->pb);
    avformat_close_input(&state.oFmtCtx);
  }
}

int process(char *buf, size_t buf_size)
{

  int result = 0;
  state.iBufData.ptr = buf;
  state.iBufData.ori_ptr = buf;
  state.iBufData.file_size = buf_size;
  state.iBufData.size = buf_size;

  // init demux
  result = init_demux_context(&state);
  if (result < 0)
    return result;

  // init remux
  result = init_remux_context(&state);
  if (result < 0)
    return result;

  result = do_remux(&state);

  if (result < 0)
    return result;

  size_t size = state.oBufferData.file_size - state.oBufferData.size;

  printf("out buffer size: %ld , use: %ld , %p \n", state.oBufferData.file_size, size, state.oBufferData.ori_ptr);

  //reset
  state.oBufferData.size = state.oBufferData.file_size;
  state.oBufferData.ptr = state.oBufferData.ori_ptr;
  clean();
  return size;
}

// init output buffer
int init_buffer(char *buf, size_t buf_size)
{
  state.oBufferData.ptr = buf;
  state.oBufferData.ori_ptr = buf;
  state.oBufferData.file_size = buf_size;
  state.oBufferData.size = buf_size;
}

#ifdef MAIN
int main(int argc, char **argv)
{

  printf("%d  ,%d ,%d , %d ,%d ,%d \n", sizeof(short), sizeof(int), sizeof(long), sizeof(float), sizeof(double), sizeof(long double));

  if (argc < 2)
  {
    printf("Usage: ./demo ts_file_path\n");
    return 0;
  }

  int result;
  char *buffer;
  char *oBuffer;
  size_t buffer_size;
  size_t oBuffer_size = 1024 * 1024 * 5; // 分配5M空间用来接收ffempg remux后的数据

  result = av_file_map(argv[1], &buffer, &buffer_size, 0, NULL);
  if (result < 0)
  {
    goto Error;
  }

  oBuffer = malloc(oBuffer_size);

  init_buffer(oBuffer, oBuffer_size);
  result = process(buffer, buffer_size);

  if (result < 0)
  {
    printf("some error \n");
    goto Error;
  }

  FILE *file = fopen("test.mp4", "wb");
  fwrite(state.oBufferData.ori_ptr, 1, result, file);
  fclose(file);

Error:
  clean();

  // free input buffer
  av_file_unmap(buffer, buffer_size);

  // free output buffer
  free(oBuffer);
}
#endif