default_target: DecodeVideo

DecodeVideo:
	clang  ff_base/base.c ff_decode_video/decode_video.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -I libs1/include -Iff_base -v -g -o demo