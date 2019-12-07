
#ifndef BASE_BASE
#define BASE_BASE

#include <time.h>
#include "structs.h"

enum Error_Code init_demuxer(char *fileName, GlobalState *gState);
enum Error_Code init_stream_decode_context(GlobalState *gState, StreamState *streamState, int streamType);
enum Error_Code init_some_gobal_state(GlobalState *gState, StreamState *videoState);
int resolve_video_frame(GlobalState *gState, AVCodecContext *decodeCtx, AVFrame *frame);
char *concat_strarray(char *sarr[], int len);
long getCurrentTime();
#endif