#include "structs.h"
#include "base.h"

static GlobalState gState;
static StreamState audioState;
static FILE *pcmFile;
static char *pcmPath = "_temp/1.pcm";

clean()
{
  if (gState.fmtCtx)
  {
    avformat_close_input(&gState.fmtCtx);
  }
  if (audioState.codecCtx)
  {
    avcodec_close(audioState.codecCtx);
    avcodec_free_context(&audioState.codecCtx);
  }
  if (gState.frame)
  {
    av_frame_free(&gState.frame);
  }
  if (gState.dst_data[0])
  {
    av_freep(&gState.dst_data[0]);
  }
  if (pcmFile)
  {
    fclose(pcmFile);
  }
}

int init_process_context()
{
  int result;

  result = init_stream_decode_context(&gState, &audioState, AVMEDIA_TYPE_AUDIO);

  if (result < 0)
  {
    return result;
  }

  gState.audioStartPts = audioState.stream->start_time;
  gState.audioTsDen = audioState.stream->time_base.den;
  gState.sampleRate = audioState.params->sample_rate;
  gState.channels = audioState.params->channels;
  return 0;
}

int decode_audio_frame()
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
      printf("end of stream....\n");
      pkt->data = NULL;
      pkt->size = 0;
    }

    if (pkt->stream_index == gState.audioStreamIndex)
    {
      resolve_audio_frame(&gState, audioState.codecCtx, frame, pkt, &decode_count);
    }

    av_packet_unref(pkt);
    av_init_packet(pkt);

  } while (!gState.endOfFile && decode_count < gState.maxDecodeOnce);

  av_packet_unref(pkt);

  if (gState.endOfFile == 1)
  {
    clean();
  }

  return decode_count;
}

void writePcm(unsigned char *buff, int size, int64_t pts)
{
  if (!pcmFile)
  {
    pcmFile = fopen(pcmPath, "a+");
  }
  fwrite(buff, 1, size, pcmFile);
}

int main(int argc, char *argv[])
{
  int result;
  char c;
  long t1;
  if (argc < 2)
  {
    printf("Usage: ./demo filePath !\n");
    return 0;
  }

  result = init_demuxer(argv[1], &gState);
  if (result < 0)
  {
    goto Error;
  }
  remove(pcmPath);
  gState.endOfFile = 0;
  gState.maxDecodeOnce = 1000;
  gState.audioCallback = &writePcm;
  av_dump_format(gState.fmtCtx, NULL, NULL, NULL);

  result = init_process_context();
  if (result < 0)
  {
    goto Error;
  }

  result = init_some_gobal_state(&gState, &audioState, AVMEDIA_TYPE_AUDIO);
  if (result < 0)
  {
    goto Error;
  }

  t1 = getCurrentTime();
  result = decode_audio_frame();
  printf("decode count:%d,take=%ld\n", result, getCurrentTime() - t1);

  printf("play pcm: ffplay -f f32le  -ac %d _temp/1.pcm\n", gState.channels);

Error:
  if (result < 0)
  {
    printf("Error Code:%d\n", result);
  }
  clean();
}
