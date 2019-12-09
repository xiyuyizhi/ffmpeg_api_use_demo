
#include "structs.h"
#include "base.h"

static GlobalState gState;
static StreamState audioState;
static FILE *pcmFile;
static char *pcmPath = "_temp/1.pcm";

static int seekToMs = 0;

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

  if (audioState.codecCtx)
  {
    avcodec_close(audioState.codecCtx);
    avcodec_free_context(&audioState.codecCtx);
  }

  if (gState.dst_data[0])
  {
    av_freep(&gState.dst_data[0]);
  }

  if (gState.fmtCtx)
  {
    avformat_close_input(&gState.fmtCtx);
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

  if (seekToMs != 0)
  {
    int64_t seek_pts = gState.audioStartPts + gState.audioTsDen * (seekToMs / 1000);
    if (av_seek_frame(gState.fmtCtx, gState.audioStreamIndex, seek_pts, AVSEEK_FLAG_BACKWARD) < 0)
    {
      printf("Seek Fail\n");
      return -1;
    }
    printf("////////// seek //////////// startTime=%lld, seekTo=%lld\n", gState.audioStartPts, seek_pts);
    avcodec_flush_buffers(audioState.codecCtx);
  };

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
  char *buffer;
  size_t buffer_size;

  if (argc < 3)
  {
    printf("Usage: ./demo filePath seekTo  !\n");
    return 0;
  }

  seekToMs = atoi(argv[2]);

  result = av_file_map(argv[1], &buffer, &buffer_size, 0, NULL);
  if (result < 0)
  {
    goto Error;
  }

  // use custom io
  result = init_demuxer_use_customio(buffer, buffer_size, &gState);
  if (result < 0)
  {
    goto Error;
  }

  remove(pcmPath);
  gState.endOfFile = 0;
  gState.maxDecodeOnce = 1000;
  gState.audioCallback = &writePcm;

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

  result = decode_audio_frame();
  printf("decode count:%d\n", result);

Error:
  if (result < 0)
  {
    printf("Error Code:%d\n", result);
  }
  clean();
  av_file_unmap(buffer, buffer_size);
}