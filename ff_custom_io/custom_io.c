
#include "structs.h"
#include "base.h"

static GlobalState gState;

clean()
{
  if (gState.frame)
  {
    av_frame_free(&gState.frame);
  }

  if (gState.fmtCtx->pb)
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
}

int main(int argc, char *argv[])
{
  int result;
  char c;
  long t1;
  char *buffer;
  size_t buffer_size;
  if (argc < 2)
  {
    printf("Usage: ./demo filePath !\n");
    return 0;
  }

  result = av_file_map(argv[1], &buffer, &buffer_size, 0, NULL);
  if (result < 0)
  {
    goto Error;
  }
  printf("file size:%.3fM\n", (float)buffer_size / 1024 / 1024);

  result = init_demuber_use_customio(buffer, buffer_size, &gState);
  if (result < 0)
  {
    goto Error;
  }

  av_dump_format(gState.fmtCtx, NULL, NULL, NULL);

Error:
  if (result < 0)
  {
    printf("Error Code:%d\n", result);
  }
  clean();
  av_file_unmap(buffer, buffer_size);
}