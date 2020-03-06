#include "structs.h"
#include "base.h"

static GlobalState gState;
static StreamState audioState;
static PcmData pcmData;
static char *pcmPath = "_temp/1.pcm";
static char *pcmPath1 = "_temp/2.pcm";

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

  pcmData.channels = gState.channels;
  pcmData.sStream = sonicCreateStream(audioState.params->sample_rate, audioState.params->channels);
  return 0;
}

int sonic_playrate_change(PcmData *pcm)
{

  int result = 0;
  int out_offset = 0;
  int channels_size = pcm->sampleSize * pcm->channels;
  char outBuffer[2048 * channels_size];
  pcm->outNbSamples = (int)(pcm->nbSamples / pcm->rate);

  sonicSetSpeed(pcm->sStream, pcm->rate);

  result = sonicWriteFloatToStream(pcm->sStream, pcm->pcmBuffer, pcm->nbSamples);
  if (result <= 0)
  {
    return -1;
  }
  sonicFlushStream(pcm->sStream);

  do
  {
    result = sonicReadFloatFromStream(pcm->sStream, outBuffer, 2048);
    if (result > 0)
    {
      memcpy(pcm->outPcmBuffer + out_offset, outBuffer, result * channels_size);
      out_offset += result * channels_size;
    }
  } while (result > 0);

  pcm->outPcmBufferSize = out_offset;

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
  result = sonic_playrate_change(&pcmData);
  if (result < 0)
  {
    printf("倍速转换出错\n");
  }

  FILE *file1 = fopen(pcmPath, "wb");
  fwrite(pcmData.pcmBuffer, 1, pcmData.pcmBufferSize, file1);

  FILE *file2 = fopen(pcmPath1, "wb");
  fwrite(pcmData.outPcmBuffer, 1, pcmData.outPcmBufferSize, file2);

  return decode_count;
}

void writePcm(unsigned char *buff, int size, int64_t pts, int nbSamples, int sampleSize)
{
  memcpy(pcmData.pcmBuffer + pcmData.pcmBufferSize, buff, size);
  pcmData.pcmBufferSize += size;
  pcmData.nbSamples += nbSamples;
  pcmData.sampleSize = sampleSize;
}

int main(int argc, char *argv[])
{
  int result;
  char c;
  long t1;
  if (argc < 3)
  {
    printf("Usage: ./demo filePath playbackRate!\n");
    return 0;
  }

  result = init_demuxer(argv[1], &gState);
  if (result < 0)
  {
    goto Error;
  }
  remove(pcmPath);
  remove(pcmPath1);
  gState.endOfFile = 0;
  gState.maxDecodeOnce = 1000;
  gState.audioCallback = &writePcm;

  pcmData.rate = atof(argv[2]);
  pcmData.pcmBuffer = malloc(1024 * 1024 * 6);
  pcmData.outPcmBuffer = malloc(1024 * 1024 * 10);
  pcmData.pcmBufferSize = 0;
  pcmData.outPcmBufferSize = 0;
  pcmData.nbSamples = 0;
  pcmData.outNbSamples = 0;

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
  printf("play pcm: ffplay -f f32le  -ac %d _temp/2.pcm\n", gState.channels);

Error:
  if (result < 0)
  {
    printf("Error Code:%d\n", result);
  }
  clean();
}
