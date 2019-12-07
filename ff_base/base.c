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

enum Error_Code init_some_gobal_state(GlobalState *gState, StreamState *videoState, int streamType)
{

  gState->frame = av_frame_alloc();

  if (!gState->frame)
  {
    return AVFRAME_ALLOC_ERROR;
  }

  av_init_packet(&gState->pkt);

  if (streamType == AVMEDIA_TYPE_VIDEO)
  {
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
  }

  return 0;
}

int resolve_video_frame(GlobalState *gState, AVCodecContext *decodeCtx, AVFrame *frame)
{
  int result = avcodec_receive_frame(decodeCtx, frame);

  if (result != 0)
  {
    printf("Error: avcodec recieve frame,%d\n", result);
    return -1;
  }

  if (frame->pict_type == 1)
  {
    printf("关键帧======:%.2f\n", (float)(frame->pts - gState->videoStartPts) / gState->videoTsDen);
  }

  sws_scale(gState->swsCtx, frame->data, frame->linesize, 0, frame->height, gState->dst_data, gState->dst_linesize);
  gState->videoCallback(gState->dst_data[0], gState->rgbaSize, frame->width, frame->height, frame->pts);
  gState->lastVideoPts = frame->pts;
  return 0;
}

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

int resolve_audio_frame(GlobalState *gState, AVCodecContext *decodeCtx, AVFrame *frame, AVPacket *pkt, int *decode_count)
{
  int got_frame = 0;
  int result = 0;

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
      (*decode_count)++;
      int data_size = av_samples_get_buffer_size(
          NULL,
          frame->channels,
          frame->nb_samples,
          frame->format,
          1);
      int sample_size = av_get_bytes_per_sample(frame->format);
      if (frame->pts != AV_NOPTS_VALUE)
      {
        gState->lastAudioPts = frame->pts;
      }
      else
      {
        gState->lastAudioPts += frame->nb_samples * gState->audioTsDen / gState->sampleRate;
      }
      uint8_t *bf = translate_pcm(data_size, frame, sample_size);
      if (gState->audioCallback)
      {
        gState->audioCallback(bf, data_size, frame->pts);
      }
      av_free(bf);
    }
    pkt->data += result;
    pkt->size -= result;
  } while (pkt->size > 0);
}