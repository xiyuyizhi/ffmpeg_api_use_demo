default_target: AvPcket

AvFormatContext:
	clang 1_about_AvFormatContext.c -lavformat -o demo

AvPcket:
	clang 2_about_AvPacket.c -lavformat -lavcodec -lswscale -o demo

clean: 
	rm demo