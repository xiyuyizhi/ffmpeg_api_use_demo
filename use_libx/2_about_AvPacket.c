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
  AVCodecParameters *codecParams = NULL;
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
      codecParams = fmt_ctx->streams[i]->codecpar;
      video_stream_index = i;
    }
    if (codec_type == AVMEDIA_TYPE_AUDIO)
    {
      audio_stream_index = i;
    }
  }

  printf("audio_stream_index=%d,video_stream_index=%d\n", audio_stream_index, video_stream_index);

  AVCodec *codec = NULL;
  AVCodecContext *ctx = NULL;
  AVFrame *frame = NULL;
  codec = avcodec_find_decoder(codecParams->codec_id);

  if (!codec)
  {
    printf("can't find decoder");
    return 0;
  }

  ctx = avcodec_alloc_context3(codec);
  if (avcodec_open2(ctx, codec, NULL) < 0)
  {
    printf("can't open decoder");
    return 0;
  };

  if (!(frame = av_frame_alloc()))
  {
    printf("can't alloc frame");
    return 0;
  };

  // read frame
  AVPacket pack;
  av_init_packet(&pack);
  int video_frames = 0;
  int audio_frames = 0;
  int read_first_video_frame = 0;

  while (av_read_frame(fmt_ctx, &pack) == 0)
  {
    if (pack.pts == AV_NOPTS_VALUE)
    {
      // printf("stream_index:%d, AV_NOPTS_VALUE,duration=%lld\n", pack.stream_index, pack.duration);
    }
    else
    {
      // printf("stream_index:%d, pts=%lld,dts=%lld,size=%d,duration=%lld\n", pack.stream_index, pack.pts, pack.dts, pack.size, pack.duration);
    }
    if (pack.stream_index == video_stream_index)
    {
      video_frames++;
      if (!read_first_video_frame)
      {
        read_first_video_frame = 1;
        int success = 0;
        int result = avcodec_decode_video2(ctx, frame, &success, &pack);
        if (success)
        {
          printf("the number of bytes used %d\n", result);
          printf("video width=%d,height=%d,pix_fmt=%d\n", ctx->width, ctx->height, ctx->pix_fmt);
          printf("  first video frame info:\n");
          printf(
              "     linesize=%d,nb_samples=%d,format=%d,key_frame=%d,pict_type=%d,Y raw data=%s\n\n",
              frame->linesize[0],
              frame->nb_samples,
              frame->format,
              frame->key_frame,
              frame->pict_type,
              frame->data[0]);
        }
      }
    }
    if (pack.stream_index == audio_stream_index)
    {
      audio_frames++;
    }
    av_packet_unref(&pack);
    av_init_packet(&pack);
  }
  av_packet_unref(&pack);
  av_frame_free(&frame);
  avcodec_close(ctx);
  avformat_close_input(&fmt_ctx);
  avcodec_free_context(&ctx);
  printf("video frames count=%d , audio frames count=%d\n", video_frames, audio_frames);
}