#ifndef BASE_STRUCTS
#define BASE_STRUCTS

#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"
#include "libswscale/swscale.h"
#include "libavutil/avutil.h"

typedef void (*VideoFrameCallback)(unsigned char *buff, int size, int width, int height, int64_t pts);
typedef void (*AudioFrameCallback)(unsigned char *buff, int size, int64_t pts);
typedef void (*VideoMetadataCallback)(int width, int height, int profile, int level, int timescale, long duration, int64_t start_time);
typedef void (*AudioMetadataCallback)(int channels, int sample_rate, int timescale, long duration, int64_t start_time);

typedef struct _BufferData
{
  uint8_t *ptr;
  uint8_t *ori_ptr;
  size_t size; // size left in buffer
  size_t file_size;
} BufferData;

typedef struct _GlobalState
{
  BufferData bd;
  AVFormatContext *fmtCtx;
  AVFrame *frame;
  AVPacket pkt;
  struct SwsContext *swsCtx;
  int videoStreamIndex;
  int audioStreamIndex;
  size_t videoTsDen;
  size_t audioTsDen;
  int64_t videoStartPts;
  int64_t audioStartPts;
  int64_t lastVideoPts;
  int64_t lastAudioPts;
  int endOfFile;
  int maxDecodeOnce;
  int rgbaSize;
  int sampleRate;
  int channels;
  uint8_t *dst_data[4];
  int dst_linesize[4];
  VideoFrameCallback videoCallback;
  AudioFrameCallback audioCallback;
  VideoMetadataCallback vMetaCallback;
  AudioMetadataCallback aMetaCallback;
} GlobalState;

typedef struct _StreamState
{
  AVCodec *codec;
  AVCodecParameters *params;
  AVCodecContext *codecCtx;
  AVStream *stream;
  int streamIndex;
} StreamState;

enum Error_Code
{
  OPEN_FILE_ERROR = -1,
  FIND_STREAM_INFO_ERROR = -2,
  FIND_BEST_STREAM_ERROR = -3,
  OPEN_DECODE_CONTEXT_ERROR = -4,
  AVFRAME_ALLOC_ERROR = -5,
  SWSCTX_ALLOC_ERROT = -6,
  AVIO_ALLOC_ERROR = -7
};

#endif