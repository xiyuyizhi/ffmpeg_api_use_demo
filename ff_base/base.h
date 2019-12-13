
#ifndef BASE_BASE
#define BASE_BASE

#define MAIN_RUN
#define USE_THREAD

#include <time.h>
#include <dirent.h>
#include "structs.h"

#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

enum Error_Code init_demuxer(char *fileName, GlobalState *gState);
enum Error_Code init_demuxer_use_customio(uint8_t *buffer, size_t buffer_size, GlobalState *gState);
enum Error_Code init_stream_decode_context(GlobalState *gState, StreamState *streamState, int streamType);
enum Error_Code init_some_gobal_state(GlobalState *gState, StreamState *videoState, int streamType);
int resolve_video_frame(GlobalState *gState, AVCodecContext *decodeCtx, AVFrame *frame);
int resolve_audio_frame(GlobalState *gState, AVCodecContext *decodeCtx, AVFrame *frame, AVPacket *pkt, int *decode_count);
long getCurrentTime();
int remove_all_temp_rgb(char *s);
#endif