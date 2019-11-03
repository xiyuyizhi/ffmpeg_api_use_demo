#include <stdio.h>
#include "libavformat/avformat.h"

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("place input a file url\n");
    return 0;
  }

  AVFormatContext *fmt_ctx = NULL;
  int video_stream_index = -1;
  int audio_stream_index = -1;

  if (avformat_open_input(&fmt_ctx, argv[1], NULL, NULL) < 0)
  {
    printf("can not open the file!\n");
    return 0;
  }

  for (int i = 0; i < fmt_ctx->nb_streams; i++)
  {
    int codec_type = fmt_ctx->streams[i]->codecpar->codec_type;
    if (codec_type == AVMEDIA_TYPE_VIDEO)
    {
      video_stream_index = i;
    }
    if (codec_type == AVMEDIA_TYPE_AUDIO)
    {
      audio_stream_index = i;
    }
  }

  printf("audio_stream_index=%d,video_stream_index=%d\n", audio_stream_index, video_stream_index);

  // read frame
  AVPacket pack;

  av_init_packet(&pack);

  while (av_read_frame(fmt_ctx, &pack) == 0)
  {
    if (pack.pts == AV_NOPTS_VALUE)
    {
      printf("stream_index:%d, AV_NOPTS_VALUE,duration=%lld\n", pack.stream_index, pack.duration);
    }
    else
    {
      printf("stream_index:%d, pts=%lld,dts=%lld,size=%d,duration=%lld\n", pack.stream_index, pack.pts, pack.dts, pack.size, pack.duration);
    }
    av_packet_unref(&pack);
    av_init_packet(&pack);
  }
}