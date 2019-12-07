#include "./base.h"

char *concat_strarray(char *sarr[], int len)
{
  char *ret = NULL;
  int size = 0;

  for (int i = 0; i < len; i++)
  {
    size += strlen(sarr[i]);
  }
  ret = malloc(size + 1);
  *ret = '\0';
  for (int i = 0; i < len; i++)
  {
    strcat(ret, sarr[i]);
  }
  return ret;
}

long getCurrentTime()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

enum Error_Code init_demuxer(char *fileName, GlobalState *gState)
{

  if (avformat_open_input(&gState->fmtCtx, fileName, NULL, NULL) < 0)
  {
    printf("can not open the file!\n");
    return OPEN_FILE_ERROR;
  }
  if (avformat_find_stream_info(gState->fmtCtx, NULL) < 0)
  {
    printf("can't find stream info");
    return FIND_STREAM_INFO_ERROR;
  }
  for (int i = 0; i < gState->fmtCtx->nb_streams; i++)
  {
    int codec_type = gState->fmtCtx->streams[i]->codecpar->codec_type;
    if (codec_type == AVMEDIA_TYPE_VIDEO)
    {
      gState->videoStreamIndex = i;
    }
    if (codec_type == AVMEDIA_TYPE_AUDIO)
    {
      gState->audioStreamIndex = i;
    }
  }
  return 0;
}

enum Error_Code init_stream_decode_context(GlobalState *gState, StreamState *streamState, int streamType)
{

  int result;
  int streamIndex;

  streamIndex = av_find_best_stream(gState->fmtCtx, streamType, -1, -1, &streamState->codec, 0);

  if (streamIndex < 0)
  {
    return FIND_BEST_STREAM_ERROR;
  }

  streamState->codecCtx = avcodec_alloc_context3(streamState->codec);
  streamState->streamIndex = streamIndex;
  streamState->stream = gState->fmtCtx->streams[streamIndex];
  streamState->params = streamState->stream->codecpar;
  if (streamType == AVMEDIA_TYPE_VIDEO)
  {
    streamState->codecCtx->thread_count = 3;
  }

  result = avcodec_parameters_to_context(streamState->codecCtx, streamState->params);
  if (result < 0)
  {
    return OPEN_DECODE_CONTEXT_ERROR;
  }
  result = avcodec_open2(streamState->codecCtx, streamState->codec, NULL);
  if (result < 0)
  {
    return OPEN_DECODE_CONTEXT_ERROR;
  }
  return 0;
}

enum Error_Code init_some_gobal_state(GlobalState *gState, StreamState *videoState)
{

  gState->frame = av_frame_alloc();

  if (!gState->frame)
  {
    return AVFRAME_ALLOC_ERROR;
  }

  av_init_packet(&gState->pkt);

  int width = videoState->params->width;
  int height = videoState->params->height;
  gState->rgbaSize = av_image_alloc(gState->dst_data, gState->dst_linesize, width, height, AV_PIX_FMT_RGBA, 1);
  gState->swsCtx = sws_getContext(
      width,
      height,
      videoState->codecCtx->pix_fmt,
      width, height,
      AV_PIX_FMT_RGBA,
      NULL,
      NULL,
      NULL,
      NULL);
  if (!gState->swsCtx)
  {
    return SWSCTX_ALLOC_ERROT;
  }
  return 0;
}

int resolve_video_frame(GlobalState *gState, AVCodecContext *decodeCtx, AVFrame *frame)
{
  int result = avcodec_receive_frame(decodeCtx, frame);

  if (result != 0)
  {
    printf("Error: avcodec recive frame,%d\n", result);
    return -1;
  }
  if (frame->pict_type == 1)
  {
    printf("关键帧======:%.2f\n", (float)(frame->pts - gState->videoStartPts) / gState->videoTsDen);
  }

  sws_scale(gState->swsCtx, frame->data, frame->linesize, 0, frame->height, gState->dst_data, gState->dst_linesize);

  return 0;
}