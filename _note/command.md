## ffmpeg

### 基本参数

```
Per-file main options:
-f fmt              force format
-c codec            codec name
-codec codec        codec name
-pre preset         preset name
-map_metadata outfile[,metadata]:infile[,metadata]  set metadata information of outfile from infile
-t duration         record or transcode "duration" seconds of audio/video
-to time_stop       record or transcode stop time
-fs limit_size      set the limit file size in bytes
-ss time_off        set the start time offset
-sseof time_off     set the start time offset relative to EOF
-seek_timestamp     enable/disable seeking by timestamp with -ss
-timestamp time     set the recording timestamp ('now' to set the current time)
-metadata string=string  add metadata
-program title=string:st=number...  add program with specified streams
-target type        specify target file type ("vcd", "svcd", "dvd", "dv" or "dv50" with optional prefixes "pal-", "ntsc-" or "film-")
-apad               audio pad
-frames number      set the number of frames to output
-filter filter_graph  set stream filtergraph
-filter_script filename  read stream filtergraph description from a file
-reinit_filter      reinit filtergraph on input parameter changes
-discard            discard
-disposition        disposition

Advanced per-file options:
-map [-]input_file_id[:stream_specifier][,sync_file_id[:stream_s  set input stream mapping
-map_channel file.stream.channel[:syncfile.syncstream]  map an audio channel from one stream to another
-map_chapters input_file_index  set chapters mapping
-accurate_seek      enable/disable accurate seeking with -ss
-itsoffset time_off  set the input ts offset
-itsscale scale     set the input ts scale
-dframes number     set the number of data frames to output
-re                 read input at native frame rate
-shortest           finish encoding within shortest input
-bitexact           bitexact mode
-copyinkf           copy initial non-keyframes
-copypriorss        copy or discard frames before start time
-tag fourcc/tag     force codec tag/fourcc
-q q                use fixed quality scale (VBR)
-qscale q           use fixed quality scale (VBR)
-profile profile    set profile
-attach filename    add an attachment to the output file
-dump_attachment filename  extract an attachment into a file
-stream_loop loop count  set number of times input stream shall be looped
-thread_queue_size  set the maximum number of queued packets from the demuxer
-find_stream_info   read and decode the streams to fill missing information with heuristics
-autorotate         automatically insert correct rotate filters
-muxdelay seconds   set the maximum demux-decode delay
-muxpreload seconds  set the initial demux-decode delay
-time_base ratio    set the desired time base hint for output stream (1:24, 1:48000 or 0.04166, 2.0833e-5)
-enc_time_base ratio  set the desired time base for the encoder (1:24, 1:48000 or 0.04166, 2.0833e-5). two special values are defined - 0 = use frame rate (video) or sample rate (audio),-1 = match source time base
-bsf bitstream_filters  A comma-separated list of bitstream filters
-fpre filename      set options from indicated preset file
-max_muxing_queue_size packets  maximum number of packets that can be buffered while waiting for all streams to initialize
-dcodec codec       force data codec ('copy' to copy stream)

Video options:
-vframes number     set the number of video frames to output
-r rate             set frame rate (Hz value, fraction or abbreviation)
-s size             set frame size (WxH or abbreviation)
-aspect aspect      set aspect ratio (4:3, 16:9 or 1.3333, 1.7777)
-bits_per_raw_sample number  set the number of bits per raw sample
-vn                 disable video
-vcodec codec       force video codec ('copy' to copy stream)
-timecode hh:mm:ss[:;.]ff  set initial TimeCode value.
-pass n             select the pass number (1 to 3)
-vf filter_graph    set video filters
-ab bitrate         audio bitrate (please use -b:a)
-b bitrate          video bitrate (please use -b:v)
-dn                 disable data

Advanced Video options:
-pix_fmt format     set pixel format
-intra              deprecated use -g 1
-rc_override override  rate control override for specific intervals
-sameq              Removed
-same_quant         Removed
-passlogfile prefix  select two pass log file name prefix
-deinterlace        this option is deprecated, use the yadif filter instead
-psnr               calculate PSNR of compressed frames
-vstats             dump video coding statistics to file
-vstats_file file   dump video coding statistics to file
-vstats_version     Version of the vstats format to use.
-intra_matrix matrix  specify intra matrix coeffs
-inter_matrix matrix  specify inter matrix coeffs
-chroma_intra_matrix matrix  specify intra matrix coeffs
-top                top=1/bottom=0/auto=-1 field first
-vtag fourcc/tag    force video tag/fourcc
-qphist             show QP histogram
-force_fps          force the selected framerate, disable the best supported framerate selection
-streamid streamIndex:value  set the value of an outfile streamid
-force_key_frames timestamps  force key frames at specified timestamps
-hwaccel hwaccel name  use HW accelerated decoding
-hwaccel_device devicename  select a device for HW acceleration
-hwaccel_output_format format  select output format used with HW accelerated decoding
-vc channel         deprecated, use -channel
-tvstd standard     deprecated, use -standard
-vbsf video bitstream_filters  deprecated
-vpre preset        set the video options to the indicated preset

Audio options:
-aframes number     set the number of audio frames to output
-aq quality         set audio quality (codec-specific)
-ar rate            set audio sampling rate (in Hz)
-ac channels        set number of audio channels
-an                 disable audio
-acodec codec       force audio codec ('copy' to copy stream)
-vol volume         change audio volume (256=normal)
-af filter_graph    set audio filters

Advanced Audio options:
-atag fourcc/tag    force audio tag/fourcc
-sample_fmt format  set sample format
-channel_layout layout  set channel layout
-guess_layout_max   set the maximum number of channels to try to guess the channel layout
-absf audio bitstream_filters  deprecated
-apre preset        set the audio options to the indicated preset

```

### 常用选项

- i 指定输入

- an 去掉音频

- vn 去掉视频

- c:v codecs | copy 指定使用的视频编码器

- c:a codecs | copy 指定使用的音频编码器

- vcodec codecs | acodec codecs 同上

- b:v | b:a 指定音视频码率

- r 指定视频帧率

- vf scale=640:360 指定视频分辨率

- crf 取值 0-51 0 为无损模式，数值越大，画质越差，生成的文件却越小

- ss 指定开始处理视频时间

- t 指定处理视频的时长

- map 指定选用的音视频轨道

### 使用

ffmpeg -i input.ts -an no-audio.mp4 // ts 转 mp4 同时去掉音频

ffmpeg -i 1.ts -vcodec libx265 -c:a copy 265.mp4 // ts h264 编码转 h265

ffmpeg -i 1.mp4 -c:v libx265 -crf 28 -c:a aac -b:a 128k 265.mp4

ffmpeg -i 1.ts -vn -acodec mp3 mp3.mp3 // 过滤视频 aac 编码 ts 转 mp3 编码

ffmpeg -i 1.ts -vf scale=640:360 1.mp4 // 修改视频分辨率

ffmpeg -i 1.ts -an -r 20 1.mp4 // 改变帧率

ffmpeg -ss 00:00:00 -t 00:00:30 -i test.mp4 -vcodec copy -acodec copy output.mp4 // 从 0s 开始截取 30s

ffmpeg -i 1.mp4 -i 2.mp4 -map 0:0 -map 1:1 out.mp4 //两个视频合并成一个，使用 1.mp4 的视频，2.mp4 的音频

### hls

```s
ffmpeg -hide_banner -y -i Animals_10___4K.mp4 \
  -vf scale=640:360:force_original_aspect_ratio=decrease -c:a aac -ar 48000 -c:v h264 -profile:v main -crf 20 -sc_threshold 0 -g 48 -keyint_min 48 -hls_time 4 -hls_playlist_type vod  -b:v 800k -maxrate 856k -bufsize 1200k -b:a 96k -hls_segment_filename ~/codeme/hls-demo/media/1080p_%03d.ts ~/codeme/hls-demo/media/1080p.m3u8 \
  -vf scale=842:480:force_original_aspect_ratio=decrease -c:a aac -ar 48000 -c:v h264 -profile:v main -crf 20 -sc_threshold 0 -g 48 -keyint_min 48 -hls_time 4 -hls_playlist_type vod -b:v 1400k -maxrate 1498k -bufsize 2100k -b:a 128k -hls_segment_filename ~/codeme/hls-demo/media/1080p_%03d.ts ~/codeme/hls-demo/media/1080p.m3u8 \
  -vf scale=1280:720:force_original_aspect_ratio=decrease -c:a aac -ar 48000 -c:v h264 -profile:v main -crf 20 -sc_threshold 0 -g 48 -keyint_min 48 -hls_time 4 -hls_playlist_type vod -b:v 2800k -maxrate 2996k -bufsize 4200k -b:a 128k -hls_segment_filename ~/codeme/hls-demo/media/1080p_%03d.ts ~/codeme/hls-demo/media/1080p.m3u8 \
  -vf scale=1920:1080:force_original_aspect_ratio=decrease -c:a aac -ar 48000 -c:v h264 -profile:v main -crf 20 -sc_threshold 0 -g 48 -keyint_min 48 -hls_time 4 -hls_playlist_type vod -b:v 5000k -maxrate 5350k -bufsize 7500k -b:a 192k -hls_segment_filename ~/codeme/hls-demo/media/1080p_%03d.ts ~/codeme/hls-demo/media/1080p.m3u8
```

ffmpeg -i 2.mp4 -f hls -hls_time 3 -hls_segment_filename 'file%03d.ts' 1.m3mu8

hls 直播流

```s
ffmpeg -re -i  1.mp4 -c copy  -hls_time 3 -hls_list_size 3 -hls_flags delete_segments+append_list+split_by_time  live/live.m3u8
```

## drm

widevine:

```s
./packager \
'in=in.mp4,stream=video,init_segment=init.mp4,segment_template=$Number$.mp4,playlist_name=index.m3u8' \
--protection_systems Widevine \
--enable_widevine_encryption \
--protection_scheme	 cenc  \
--key_server_url https://license.uat.widevine.com/cenc/getcontentkey/widevine_test \
--content_id 736b646c666d786a646f736964667867 \
--signer widevine_test \
--aes_signing_key 1ae8ccd0e7985cc0b6203a55855a1034afc252980e970ca90e5202689f947ab9 \
--aes_signing_iv d58ce954203b7c9a9a9d467f59839249 \
--segment_duration 3 \
--hls_master_playlist_output master.m3u8
```

clearkey:

```s
./packager \
'in=in.mp4,stream=video,init_segment=init.mp4,segment_template=$Number$.mp4,playlist_name=index.m3u8,drm_label=SD' \
--enable_raw_key_encryption \
--keys label=SD:key_id=abba271e8bcf552bbd2e86a434a9a5d9:key=69eaa802a6763af979e8d1940fb88392 \
--hls_master_playlist_output h264_master.m3u8
```

## mp4box

## DASH

[https://github.com/gpac/gpac/wiki/DASH-Introduction](https://github.com/gpac/gpac/wiki/DASH-Introduction)

- rap

  > 强制 segment 按 IDR、R 帧分割

- segment-name

- segment-ext

  > 指定 segment 的后缀名

  ```js
      MP4box -frag 2000 -dash 5000 -segment-name segment_ -segment-ext mp4 inp.mp4
      MP4box -frag 2000 -dash 5000 -segment-name segment_ -segment-ext mp4 -out segment_splited/dashmpd 1.mp4

  ```

- Fragmentation -frag 单位 ms

  > 指定 fragment 的 duration ,以 duration 时长为一组 fragment


    ```js
    MP4box -frag 2000 inp.mp4
    moov mdat 格式转换成 | moov | moof | mdat | moof | mdat | moof | mdat| .....
    // 每组 moof mdat 包含两秒数据
    ```

- Segmentation -dash 单位 ms

  > creating segments, parts of an original file meant for individual/separate HTTP download (not necessarily for individual playback). A segment can be a part of a big file or a separate file.

  ```js
      MP4box -frag 2000 -dash 5000 inp.mp4
      //moov mdat 格式转换成 `2s一组`连续moof+mdat, `每5s插入sidx box`,5s一组的 segment。默认对segment不生成独立的segment文件, `同时生成 mpd清单文件`
                               |-------------------- segment ------------|      | -------- segment -----
                               |-- frgament -|-- fragment -|
      | moov mdat |  -> | moov | moof | mdat | moof | mdat | moof | mdat | sidx | moof | mdat | .....


      MP4box -frag 2000 -dash 5000 -segment-name segment_ inp.mp4

      -segment-name

          如果不指定，segment 自动拼接到一个生成文件中 mpd中 <SegmentURL mediaRange="1329-594006" indexRange="1329-1396"/>
          `indexRange: 指定 sidx box的位置`

          指定的话，生成单独的segment文件
              <SegmentList timescale="1000" duration="5000">
                  <Initialization sourceURL="segment_fileinit.mp4"/>
                  <SegmentURL media="segment_file1.m4s"/>
                  <SegmentURL media="segment_file2.m4s"/>
                  <SegmentURL media="segment_file3.m4s"/>
              </SegmentList>

  ```

  ```js
  (
    <Period duration="PT0H0M10.040S">
      <AdaptationSet
        segmentAlignment="true"
        maxWidth="640"
        maxHeight="360"
        maxFrameRate="12800/514"
        par="16:9"
        lang="und"
      >
        <ContentComponent id="1" contentType="video" />
        <ContentComponent id="2" contentType="audio" />
        <Representation
          id="1"
          mimeType="video/mp4"
          codecs="avc3.64001E,mp4a.40.2"
          width="640"
          height="360"
          frameRate="25"
          sar="1:1"
          audioSamplingRate="2"
          startWithSAP="0"
          bandwidth="850006"
        >
          <AudioChannelConfiguration
            schemeIdUri="urn:mpeg:dash:23003:3:audio_channel_configuration:2011"
            value="2"
          />
          <BaseURL>1_dashinit.mp4</BaseURL>
          <SegmentList timescale="1000" duration="5000">
            <Initialization range="0-1328" />
            <SegmentURL mediaRange="1329-594006" indexRange="1329-1396" />
            <SegmentURL
              mediaRange="594007-1061428"
              indexRange="594007-594074"
            />
            <SegmentURL
              mediaRange="1061429-1066757"
              indexRange="1061429-1061472"
            />
          </SegmentList>
        </Representation>
      </AdaptationSet>
    </Period>
  )`SegmentList 标签`;
  ```

* 多清晰度

  ```js

      MP4Box -dash 10000 -frag 10000 -rap -out out.mpd 360.mp4 720.mp4

  ```
