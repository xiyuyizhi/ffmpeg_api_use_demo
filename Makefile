default_target: DecodeVideo

DecodeVideo:
	clang ff_decode_video/decode_video.c  -lavformat -lavcodec  -Llibs1/lib -Ilibs1/include -o demo