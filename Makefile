default_target: DecodeVideo

DecodeVideo:
	clang  ff_base/base.c ff_decode_video/decode_video.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base -v -g -o demo

DecodeAudio:
	clang  ff_base/base.c ff_decode_audio/decode_audio.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base -v -g -o demo