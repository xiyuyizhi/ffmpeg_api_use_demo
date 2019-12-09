#include "structs.h"
#include "base.h"

#define MAIN_RUN
#define USE_THREAD

static GlobalState gState;
static StreamState videoState;
static StreamState audioState;

#ifdef MAIN_RUN
static FILE *pcmFile;
static char *pcmPath = "_temp/1.pcm";
#endif

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
#ifdef MAIN_RUN
  if (pcmFile)
  {
    fclose(pcmFile);
  }
#endif
}

int init_callback(long vMeta, long aMeta, long v, long a)
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

#ifdef USE_THREAD
  videoState.codecCtx->thread_count = 3;
#endif

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
  if (result < 0)
    return result;

  result = init_some_gobal_state(&gState, &videoState, AVMEDIA_TYPE_VIDEO);
  return result;
}

int frame_process()
{
  int decode_count = 0;
  int result = 0;
  AVPacket *pkt = &gState.pkt;
  AVFrame *frame = gState.frame;

  do
  {
    if (!gState.endOfFile)
    {
      if (av_read_frame(gState.fmtCtx, pkt) < 0)
      {
        gState.endOfFile = 1;
      }
    }

    if (gState.endOfFile == 1)
    {
      printf("mode: %d, end of stream....\n", gState.decodeMode);
      pkt->data = NULL;
      pkt->size = 0;
    }

    if (gState.decodeMode == 1 && pkt->stream_index == gState.videoStreamIndex)
    {
      result = avcodec_send_packet(videoState.codecCtx, pkt);
      if (result != 0)
      {
        printf("Error: avcodec_send_packet,%d\n", result);
      }
      else if (resolve_video_frame(&gState, videoState.codecCtx, frame) >= 0)
      {
        printf("%lld \n", frame->pts);
        decode_count++;
      }
      if (gState.endOfFile)
      {
        //flush frame
        while (result == 0)
        {
          result = resolve_video_frame(&gState, videoState.codecCtx, frame);
          decode_count++;
        }
      }
    }

    if (gState.decodeMode == 2 && pkt->stream_index == gState.audioStreamIndex)
    {
      resolve_audio_frame(&gState, audioState.codecCtx, frame, pkt, &decode_count);
    }

    av_packet_unref(pkt);
    av_init_packet(pkt);

  } while (!gState.endOfFile && decode_count < gState.maxDecodeOnce);

  av_packet_unref(pkt);

  return decode_count;
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
  printf("Video Metadata: width=%d,height=%d,profile=%d,level=%d,timescale=%d,start_time=%lld\n",
         width,
         height,
         profile,
         level,
         timescale,
         start_time);
}

void aMetaCallback(int channels, int sample_rate, int timescale, int64_t start_time)
{
  printf("Audio Metadata: channels=%d,sample_rate=%d,timescle=%d,start_time=%lld\n",
         channels,
         sample_rate,
         timescale,
         start_time);
}

int main(int argc, char *argv[])
{
  if (argc < 4)
  {
    printf("Usage: ./demo filePath mode decodeCount! \n");
    return 0;
  }

  remove_all_temp_rgb("_temp/");

  char *buffer;
  size_t buffer_size;
  int result;
  int mode = atoi(argv[2]);

  gState.maxDecodeOnce = atoi(argv[3]);

  result = av_file_map(argv[1], &buffer, &buffer_size, 0, NULL);
  if (result < 0)
  {
    goto Error;
  }

  init_callback(&vMetaCallback, &aMetaCallback, &vFrameCallback, &aFrameCallback);

  result = init(buffer, buffer_size, mode);
  if (result < 0 || mode == 3)
  {
    goto Error;
  }

  result = frame_process();
  printf("decode count:%d\n", result);

Error:
  if (result < 0)
  {
    printf("Error code:%d\n", result);
  }
  av_file_unmap(buffer, buffer_size);
  clean();
}
#endif

// ./demo _media/1.ts 1 10  decode 10 video frame
// ./demo _media/1.ts 2 1000  decode 100 audio frame
// ./demo _media/1.ts 3 0  no decode frame,only dump metadata info

// exported wasm method
// 1.init_callback() 2.init() 3.frame_process() 4.clean()