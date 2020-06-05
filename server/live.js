const MAX_BUFFER_SIZE = 1024 * 1024 * 2;
const MAX_BUFFER_CHACHE = 1024 * 200;

const cvs = document.querySelector('#player');
const worker = new Worker('./live-worker.js');
let first = true;
let url = '';

worker.addEventListener('message', (e) => {
  const { type } = e.data;
  switch (type) {
    case 'wasmInit':
      worker.postMessage({
        type: 'init',
      });
      start(url);
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

        if (totalOnce >= MAX_BUFFER_CHACHE) {
          do_buffer_append(bufferStore);
          bufferStore = [];
          totalOnce = 0;
        }

        if (total > MAX_BUFFER_SIZE) {
          console.log('fetch end!');
          abort.abort();
        }
        return _read();
      });
    }
    return _read();
  }

  fetch(url, { signal: abort.signal })
    .then((res) => readStream(res.body.getReader()))
    .then(() => {
      worker.postMessage({
        type: 'clean',
      });
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
