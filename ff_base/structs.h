#include "libavformat/avformat.h"
#include "libavcodec/avcodec.h"

typedef struct _GlobalState
{
  AVFormatContext *fmtCtx;
  AVFrame *frame;
  AVPacket pkt;
} GlobalState;

typedef struct _StreamState
{
  AVCodec *codec;
  AVCodecParameters *params;
  AVCodecContext *codecCtx;
  AVStream *stream;
  int streamIndex;
} StreamState;

typedef struct _BufferData
{
  uint8_t *ptr;
  uint8_t *ori_ptr;
  size_t size; // size left in buffer
  size_t file_size;
} BufferData;
