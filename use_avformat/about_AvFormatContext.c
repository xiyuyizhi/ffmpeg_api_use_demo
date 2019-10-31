
#include <stdio.h>
#include "libavformat/avformat.h"

int main(int argc, char *argv[])
{

  if (argc < 2)
  {
    printf("place input a file url\n");
    return 0;
  }

  AVFormatContext *fmt_context = NULL;
  AVInputFormat *inp_format = NULL;
  if (avformat_open_input(&fmt_context, argv[1], NULL, NULL) < 0) // The codecs are not opened
  {
    printf("can not open the file!\n");
    return 0;
  }

  if (avformat_find_stream_info(fmt_context, NULL) < 0)
  {
    printf("can not find stream information!\n");
    return 0;
  }

  av_dump_format(fmt_context, 0, argv[1], 0);

  inp_format = fmt_context->iformat;

  printf("\ninput format:\n");
  printf("  name=%s , long_name=%s , \n", inp_format->name, inp_format->long_name);
  printf("nb_streams=%d\n", fmt_context->nb_streams);

  for (int i = 0; i < fmt_context->nb_streams; i++)
  {
    AVStream *s = fmt_context->streams[i];
    printf("  stream info: index=%d,id=%d,codec_type=%d,codec_id=%d,"
           "width=%d,height=%d\n",
           s->index,
           s->id,
           s->codecpar->codec_type,
           s->codecpar->codec_id,
           s->codecpar->width,
           s->codecpar->height); //AVMEDIA_TYPE_VIDEO AVMEDIA_TYPE_AUDIO
  }

  printf("AVMEDIA_TYPE_VIDEO=%d,AVMEDIA_TYPE_AUDIO=%d\n", AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO);
  printf("AV_CODEC_ID_H264 =%d,AV_CODEC_ID_AAC =%d\n", AV_CODEC_ID_H264, AV_CODEC_ID_AAC);

  printf("nb_programs=%d\n", fmt_context->nb_programs);

  avformat_close_input(&fmt_context);
}