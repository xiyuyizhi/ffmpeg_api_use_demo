#include "structs.h"
#include "base.h"

#define MAIN_RUN
#define USE_THREAD

static GlobalState gState;
static StreamState videoState;
static StreamState audioState;

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
}

int initCallback(long vMeta, long aMeta, long v, long a)
{
  gState.videoCallback = v;
  gState.audioCallback = a;
  gState.vMetaCallback = vMeta;
  gState.aMetaCallback = aMeta;
}

int init_video_context()
{
}

int init_audio_context()
{
}

int init_process_context()
{
}

enum Error_Code init(char *buffer, size_t buffer_size, int decodeTyep)
{
  int result;
  result = init_demuber_use_customio(buffer, buffer_size, &gState);
  if (result < 0)
    return result;
}

#ifdef MAIN_RUN

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
  result = av_file_map(argv[1], &buffer, &buffer_size, 0, NULL);
  if (result < 0)
  {
    goto Error;
  }

  result = init(buffer, buffer_size, 1);
  if (result < 0)
  {
    goto Error;
  }

  av_dump_format(gState.fmtCtx, NULL, NULL, NULL);

Error:
  av_file_unmap(buffer, buffer_size);
  clean();
}
#endif