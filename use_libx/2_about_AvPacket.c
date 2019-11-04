#include <stdio.h>
#include <string.h>
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"

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
    return -1;
  }
  if (avformat_find_stream_info(fmt_ctx, NULL) < 0)
  {
    printf("can't find stream info");
    return -1;
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
  struct SwsContext *sws = NULL;

  codec = avcodec_find_decoder(codecParams->codec_id);

  if (!codec)
  {
    printf("can't find decoder");
    return -1;
  }

  ctx = avcodec_alloc_context3(codec);

  if (avcodec_parameters_to_context(ctx, codecParams))
  {
    printf("Can't copy decoder context");
    return -1;
  }

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

  printf("video width=%d,height=%d,pix_fmt=%d\n", ctx->width, ctx->height, ctx->pix_fmt);

  // read frame
  AVPacket pack;
  uint8_t *dst_data[4];
  int dst_linesize[4];
  int video_frames = 0;
  int audio_frames = 0;
  int buffer_size = 0;
  int read_first_video_frame = 0;

  av_init_packet(&pack);
  frame = av_frame_alloc();

  sws = sws_getContext(ctx->width,
                       ctx->height,
                       ctx->pix_fmt,
                       ctx->width,
                       ctx->height,
                       AV_PIX_FMT_RGB24,
                       SWS_BILINEAR,
                       NULL,
                       NULL,
                       NULL);

  buffer_size = av_image_alloc(dst_data, dst_linesize, ctx->width, ctx->height, AV_PIX_FMT_RGB24, 1);
  printf("buffer_size=%d\n", buffer_size);
  if (buffer_size < 0)
  {
    printf("Could not allocate destination image\n");
    return -1;
  }

  while (av_read_frame(fmt_ctx, &pack) == 0)
  {
    if (pack.stream_index == video_stream_index)
    {
      if (!read_first_video_frame)
      {
        read_first_video_frame = 1;
        int success = 0;
        int result = avcodec_decode_video2(ctx, frame, &success, &pack);
        if (success)
        {
          printf("  first video frame info:\n");
          printf(
              "     linesize=%d,nb_samples=%d,format=%d,key_frame=%d,pict_type=%d\n",
              frame->linesize[0],
              frame->nb_samples,
              frame->format,
              frame->key_frame,
              frame->pict_type);

          sws_scale(sws, (const char *const *)frame->data,
                    frame->linesize, 0, ctx->height,
                    dst_data, dst_linesize);
          write_to_file(dst_data[0], buffer_size, video_frames);
        }
      }
      video_frames++;
    }
    if (pack.stream_index == audio_stream_index)
    {
      audio_frames++;
    }
    av_packet_unref(&pack);
    av_init_packet(&pack);
  }
  av_freep(&dst_data[0]);
  av_packet_unref(&pack);
  av_frame_free(&frame);
  avcodec_close(ctx);
  avformat_close_input(&fmt_ctx);
  avcodec_free_context(&ctx);
  printf("video frames count=%d , audio frames count=%d\n", video_frames, audio_frames);
}

int write_to_file(uint8_t s[], int buffer_len, int fileIndex)
{
  char *concat_strarray(char *sarr[], int len);
  char *str_index;

  sprintf(str_index, "%d", fileIndex);

  FILE *file;
  char *name_parts[3] = {
      "../out/frame",
      str_index,
      ".rgb"};
  char *file_name = concat_strarray(name_parts, 3);

  file = fopen(file_name, "wb");

  fwrite(s, 1, buffer_len, file);
  fclose(file);
  free(file_name);
}

char *concat_strarray(char *sarr[], int len)
{
  char *ret = NULL;
  int size = 0;

  for (int i = 0; i < len; i++)
  {
    size += strlen(sarr[i]);
  }
  ret = malloc(size + 1);
  *ret = '\0';
  for (int i = 0; i < len; i++)
  {
    strcat(ret, sarr[i]);
  }
  return ret;
}