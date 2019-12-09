#include "structs.h"
#include "base.h"

#define MAIN_RUN
#define USE_THREAD

static GlobalState gState;
static StreamState videoState;
static StreamState audioState;
static FILE *pcmFile;
static char *pcmPath = "_temp/1.pcm";

clean()
{
  if (gState.frame)
  {
    av_frame_free(&gState.frame);
  }

  if (gState.fmtCtx && gState.fmtCtx->pb)
  {
    av_free(gState.fmtCtx->pb->buffer);
    avio_context_free(&gState.fmtCtx->pb);
  }

  if (gState.dst_data[0])
  {
    av_freep(&gState.dst_data[0]);
  }

  if (gState.fmtCtx)
  {
    avformat_close_input(&gState.fmtCtx);
  }

  if (audioState.codecCtx)
  {
    avcodec_close(audioState.codecCtx);
    avcodec_free_context(&audioState.codecCtx);
  }

  if (videoState.codecCtx)
  {
    avcodec_close(videoState.codecCtx);
    avcodec_free_context(&videoState.codecCtx);
  }
  if (pcmFile)
  {
    fclose(pcmFile);
  }
}

int initCallback(long vMeta, long aMeta, long v, long a)
{
  gState.videoCallback = v;
  gState.audioCallback = a;
  gState.vMetaCallback = vMeta;
  gState.aMetaCallback = aMeta;
}

int init_video_context(int *result)
{

  (*result) = init_stream_decode_context(&gState, &videoState, AVMEDIA_TYPE_VIDEO);

  if (*result < 0)
  {
    return -1;
  }

  gState.videoStartPts = videoState.stream->start_time;
  gState.videoTsDen = videoState.stream->time_base.den;

  if (gState.decodeMode == 3)
  {
    gState.vMetaCallback(
        videoState.params->width,
        videoState.params->height,
        videoState.params->profile,
        videoState.params->level,
        gState.videoTsDen,
        gState.videoStartPts);
  }

  return 0;
}

int init_audio_context(int *result)
{

  (*result) = init_stream_decode_context(&gState, &audioState, AVMEDIA_TYPE_AUDIO);

  if (*result < 0)
  {
    return -1;
  }

  gState.audioStartPts = audioState.stream->start_time;
  gState.audioTsDen = audioState.stream->time_base.den;
  gState.sampleRate = audioState.params->sample_rate;
  gState.channels = audioState.params->channels;
  if (gState.decodeMode == 3)
  {
    gState.aMetaCallback(gState.channels, gState.sampleRate, gState.audioTsDen, gState.audioStartPts);
  }
  return 0;
}

enum Error_Code init_process_context()
{
  int result = 0;
  init_video_context(&result);
  if (result < 0)
    return result;
  init_audio_context(&result);
  return result;
}

enum Error_Code init(char *buffer, size_t buffer_size, int decodeMode)
{
  int result;

  result = init_demuxer_use_customio(buffer, buffer_size, &gState);

  if (result < 0)
    return result;

  gState.decodeMode = decodeMode;

  result = init_process_context();
  return result;
}

#ifdef MAIN_RUN

void vFrameCallback(unsigned char *buff, int size, int width, int height, int64_t pts)
{
  char *s[20];
  char *s1 = "_temp/";
  char *s2 = ".rgb";
  sprintf(s, "%lld", pts);
  char *filePath = malloc(strlen(s1) + strlen(s) + strlen(s2) + 1);
  strcpy(filePath, s1);
  strcat(filePath, s);
  strcat(filePath, s2);

  FILE *file = fopen(filePath, "wb");
  fwrite(buff, 1, size, file);
  fclose(file);
  free(filePath);
}

void aFrameCallback(unsigned char *buff, int size, int64_t pts)
{
  if (!pcmFile)
  {
    pcmFile = fopen(pcmPath, "a+");
  }
  fwrite(buff, 1, size, pcmFile);
}

void vMetaCallback(int width, int height, int profile, int level, int timescale, int64_t start_time)
{
  printf("video Metadata: width=%d,height=%d,profile=%d,level=%d,timescale=%d,start_time=%lld\n",
         width,
         height,
         profile,
         level,
         timescale,
         start_time);
}

void aMetaCallback(int channels, int sample_rate, int timescale, int64_t start_time)
{
  printf("audio Metadata: channels=%d,sample_rate=%d,timescle=%d,start_time=%lld\n",
         channels,
         sample_rate,
         timescale,
         start_time);
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Usage: ./demo filePath xxx! \n");
    return 0;
  }

  char *buffer;
  size_t buffer_size;
  int result;
  int mode = 3;
  result = av_file_map(argv[1], &buffer, &buffer_size, 0, NULL);
  if (result < 0)
  {
    goto Error;
  }

  initCallback(&vMetaCallback, &aMetaCallback, &vFrameCallback, &aFrameCallback);

  result = init(buffer, buffer_size, mode);
  if (result < 0)
  {
    goto Error;
  }

Error:
  if (result < 0)
  {
    printf("Error code:%d\n", result);
  }
  av_file_unmap(buffer, buffer_size);
  clean();
}
#endif