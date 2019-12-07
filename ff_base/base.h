
#ifndef BASE_BASE
#define BASE_BASE

#include <time.h>
#include "structs.h"

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

enum Error_Code init_demuxer(char *fileName, GlobalState *gState);
enum Error_Code init_stream_decode_context(GlobalState *gState, StreamState *streamState, int streamType);
enum Error_Code init_some_gobal_state(GlobalState *gState, StreamState *videoState, int streamType);
int resolve_video_frame(GlobalState *gState, AVCodecContext *decodeCtx, AVFrame *frame);
int resolve_audio_frame(GlobalState *gState, AVCodecContext *decodeCtx, AVFrame *frame, AVPacket *pkt, int *decode_count);
char *concat_strarray(char *sarr[], int len);
long getCurrentTime();
#endif