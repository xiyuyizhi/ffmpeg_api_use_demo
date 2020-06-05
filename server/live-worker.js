importScripts('../libs/live-decode.js');

function video_meta_callback(width, height, timescale, startTime) {
  console.log(
    `width=${width} , height=${height} , timescale=${timescale}  , startTime=${startTime}`
  );
}

function video_frame_callback(bf, u, v, width, height, size, pts) {
  const rgb = Module.HEAPU8.subarray(bf, bf + size);
  const buffer = new Uint8Array(rgb);
  console.log(width, height, size, pts);
}

const Module = m({
  locateFile: (suffix) => {
    if (/\.wasm$/.test(suffix)) {
      return location.origin + '/libs/live-decode.wasm';
    }
  },
});
console.log(Module);
const cb1 = Module.addFunction(video_meta_callback);
const cb2 = Module.addFunction(video_frame_callback);

let ptr;

Module.onRuntimeInitialized = () => {
  self.postMessage({ type: 'wasmInit' });
};

self.addEventListener('message', (e) => {
  const { type, data } = e.data;

  switch (type) {
    case 'init':
      ptr = Module._malloc(1024 * 1024);
      if (!ptr) {
        throw new Error('live worker alloc buffer fail');
      }
      Module._init_process(cb1, cb2);
      break;
    case 'chunk':
      Module.HEAPU8.set(data.buffer, ptr);
      if (data.first) {
        const result = Module._do_demuxer(ptr, data.buffer.length);
        console.log('call do_demuxer', result);
      } else {
        Module._do_append_buffer(ptr, data.buffer.length);
      }
      Module._do_read_pkts(0);
      Module._decodeVideoFrame();
      break;
    case 'clean':
      Module._clean();
      break;
  }
});
