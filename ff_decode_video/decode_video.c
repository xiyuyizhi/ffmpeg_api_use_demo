#include "structs.h"
#include "base.h"

static GlobalState gState;
static StreamState videoState;

clean()
{
  if (gState.fmtCtx)
  {
    avformat_close_input(&gState.fmtCtx);
  }
  if (videoState.codecCtx)
  {
    avcodec_close(videoState.codecCtx);
    avcodec_free_context(&videoState.codecCtx);
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

  result = init_stream_decode_context(&gState, &videoState, AVMEDIA_TYPE_VIDEO);

  if (result < 0)
  {
    return result;
  }

  gState.videoStartPts = videoState.stream->start_time;
  gState.videoTsDen = videoState.stream->time_base.den;

  return 0;
}

int decode_video_frame()
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

    if (pkt->stream_index == gState.videoStreamIndex)
    {
      result = avcodec_send_packet(videoState.codecCtx, pkt);
      if (result != 0)
      {
        printf("Error: avcodec_send_packet,%d\n", result);
      }
      else if (resolve_video_frame(&gState, videoState.codecCtx, frame) >= 0)
      {
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

    av_packet_unref(pkt);
    av_init_packet(pkt);

  } while (!gState.endOfFile && decode_count < gState.maxDecodeOnce);

  return decode_count;
}

int main(int argc, char *argv[])
{
  int result;
  char c;
  long t1;
  if (argc < 3)
  {
    printf("Usage: ./demo filePath maxDecodeOnce !\n");
    return 0;
  }

  result = init_demuxer(argv[1], &gState);
  if (result < 0)
  {
    goto Error;
  }
  gState.endOfFile = 0;
  gState.maxDecodeOnce = atoi(argv[2]);

  av_dump_format(gState.fmtCtx, "", NULL, NULL);

  result = init_process_context();
  if (result < 0)
  {
    goto Error;
  }

  result = init_some_gobal_state(&gState, &videoState);
  if (result < 0)
  {
    goto Error;
  }

  t1 = getCurrentTime();
  result = decode_video_frame();
  printf("decode count:%d,take=%d\n", result, getCurrentTime() - t1);

  while ((c = getchar()) != '0' && result == gState.maxDecodeOnce)
  {
    t1 = getCurrentTime();
    result = decode_video_frame();
    printf("decode count:%d,take=%d\n", result, getCurrentTime() - t1);
  }

Error:
  if (result < 0)
  {
    printf("Error Code:%d\n", result);
  }
  clean();
}