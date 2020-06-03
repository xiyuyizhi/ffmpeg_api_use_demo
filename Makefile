default_target: FlvLive

FlvLive:
	clang  ff_base/base.c ff_live/flv_live.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base -v -g -o demo

DecodeVideo:
	clang  ff_base/base.c ff_decode_video/decode_video.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base -v -g -o demo

DecodeAudio:
	clang  ff_base/base.c ff_decode_audio/decode_audio.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base -v -g -o demo

AudioSonic:
	clang  ff_base/base.c ff_base/sonic.c ff_playbackrate/decode_audio.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base -v -g -o demo

CustomIo:
	clang  ff_base/base.c ff_custom_io/custom_io.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base -v -g -o demo

Seek:
	clang  ff_base/base.c ff_seek/seek.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base -v -g -o demo

Wasm:
	clang  ff_base/base.c ff_wasm/decode.c  -lavutil  -lavformat -lavcodec  -lswscale -liconv -lz  -Llibs1/lib -Ilibs1/include -Iff_base  -v -g -o demo

Remux:
	clang  ff_remux/remux.c  -lavutil  -lavformat -lavcodec -liconv -lz  -Llibs1/lib -Ilibs1/include -v -g -o demo


# -framework VideoToolbox -framework CoreVideo