const MAX_BUFFER_SIZE = 1024 * 1024 * 5;
const MAX_BUFFER_CHACHE = 1024 * 250;
const MAX_BUFFER_CHACHE_FIRST = 1024 * 450;

const frameQueue = [];
const worker = new Worker('./live-worker.js');
let draw = null;
let timer = null;
let first = true;
let url =
  'https://dno-xiu-hd.youku.com/lfgame/stream_alias_1771825760_8077115_4lf720.flv?auth_key=1623335958-0-0-9b5b808d52eb1f0912c92d04ac9e1804';

window.frameQueue = frameQueue;

worker.addEventListener('message', (e) => {
  const { type } = e.data;
  switch (type) {
    case 'wasmInit':
      worker.postMessage({
        type: 'init',
      });
      start(url);
      break;
    case 'metadata':
      draw = initCanvas(e.data.metaInfo);
      break;
    case 'frame':
      frameQueue.push(e.data.frameInfo);
      if (timer) return;
      startRender();
      break;
    case 'decodeFinish':
      break;
  }
});

function start(url) {
  let bufferStore = [];
  let total = 0;
  let totalOnce = 0;
  let abort = new AbortController();

  function readStream(reader) {
    function _read() {
      return reader.read().then(({ value, done }) => {
        total += value.byteLength;
        totalOnce += value.byteLength;
        bufferStore.push(value);

        if (
          totalOnce >= (first ? MAX_BUFFER_CHACHE_FIRST : MAX_BUFFER_CHACHE) &&
          frameQueue.length < 10
        ) {
          do_buffer_append(bufferStore);
          bufferStore = [];
          totalOnce = 0;
        }

        if (total > MAX_BUFFER_SIZE) {
          console.warn('fetch end!!!');
          abort.abort();
          return;
        }
        return _read();
      });
    }
    return _read();
  }

  fetch(url, { signal: abort.signal })
    .then((res) => readStream(res.body.getReader()))
    .then(() => {
      console.log('-----------------');
      worker.postMessage({
        type: 'clean',
      });
      clearInterval(timer);
      timer = null;
    });
}

function do_buffer_append(chunkList) {
  let len = chunkList.reduce((all, c) => {
    all += c.byteLength;
    return all;
  }, 0);

  let final = new Uint8Array(len);
  let offset = 0;
  console.log('new chunk, is first?  ', len, first);
  chunkList.forEach((buffer) => {
    final.set(buffer, offset);
    offset += buffer.byteLength;
  });

  worker.postMessage(
    {
      type: 'chunk',
      data: {
        first,
        buffer: final,
      },
    },
    [final.buffer]
  );
  first = false;
}

function initCanvas({ width, height }) {
  const cvs = document.querySelector('#player');
  const ctx = cvs.getContext('2d');
  cvs.width = width;
  cvs.height = height;
  const imageData = ctx.getImageData(0, 0, width, height);
  return {
    ctx,
    imageData,
  };
}

function startRender() {
  const ctx = draw.ctx;
  const cvsImgData = draw.imageData.data;

  let _render = function () {
    let frame = frameQueue.shift();
    if (!frame) {
      console.warn('lack frame!');
      clearInterval(timer);
      timer = null;
      return;
    }

    let { rgba, pts } = frame;
    for (let i = 0, len = rgba.length; i < len; i++) {
      cvsImgData[i] = rgba[i];
    }
    ctx.putImageData(draw.imageData, 0, 0);
    console.log('render: pts = ', pts, ' rest frame:', frameQueue.length);
    rgba = null;
  };

  timer = setInterval(_render, 50);
}
