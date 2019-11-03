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
  printf("  name=%s , long_name=%s  \n", inp_format->name, inp_format->long_name);
  printf("nb_streams=%d\n", fmt_context->nb_streams);

  for (int i = 0; i < fmt_context->nb_streams; i++)
  {
    AVStream *s = fmt_context->streams[i];
    AVCodecParameters *p = s->codecpar;
    // AVCodecParameters
    printf("  stream info: index=%d,id=%d,time_base.num=%d,time_base.en=%d,codec_type=%d,codec_id=%d,"
           ",nb_side_data=%d,width=%d,height=%d,"
           "bit_rate=%lld,profile=%d,level=%d,channels=%d,frame_size=%d,sample_rate=%d\n",
           s->index,
           s->id,
           s->time_base.num,
           s->time_base.den,
           p->codec_type,
           p->codec_id,
           s->nb_side_data,
           p->width,
           p->height,
           p->bit_rate,
           p->profile,
           p->level,
           p->channels,
           p->frame_size,
           p->sample_rate); //AVMEDIA_TYPE_VIDEO AVMEDIA_TYPE_AUDIO
    printf("    media type=%s\n", av_get_media_type_string(p->codec_type));

    // metadata
    AVDictionaryEntry *prev = NULL;
    printf("    metadata:count=%d\n", av_dict_count(s->metadata));
    while ((prev = av_dict_get(s->metadata, "", prev, AV_DICT_IGNORE_SUFFIX)) != NULL)
    {
      printf("      key=%s,value=%s\n", prev->key, prev->value);
    }
    av_dict_free(&(s->metadata));

    // find decoder for stream
    printf("    对应的解码器信息:\n");
    AVCodec *dec = avcodec_find_decoder(p->codec_id);
    printf("        -> codec name=%s,long_name=%s\n", dec->name, dec->long_name);
  }

  printf("AVMEDIA_TYPE_VIDEO=%d,AVMEDIA_TYPE_AUDIO=%d\n", AVMEDIA_TYPE_VIDEO, AVMEDIA_TYPE_AUDIO);
  printf("AV_CODEC_ID_H264=%d,AV_CODEC_ID_AAC=%d,AV_CODEC_ID_HEVC=%d\n", AV_CODEC_ID_H264, AV_CODEC_ID_AAC, AV_CODEC_ID_HEVC);

  printf("nb_programs=%d\n", fmt_context->nb_programs);

  avformat_close_input(&fmt_context);
}