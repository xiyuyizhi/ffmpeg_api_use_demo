self.importScripts("./remux.js")

try {
  Module = m();
} catch (e) {
  self.postMessage({type: "error", data: e.message})
}

Module.onRuntimeInitialized = () => {
  self.postMessage({type: "wasmInit"})
}

let outPtr;
let outputSize;
self.addEventListener("message", e => {
  const {type} = e.data;
  switch (type) {
    case "malloc":
      outputSize = 1024 * 1024 * 5; // alloc 5M for transfer data
      outPtr = Module._malloc(outputSize);
      Module._init_buffer(outPtr, outputSize);
      console.log('init out buffer,ptr:', outPtr);
      break;

    case "remux":
      const buffer = e.data.buffer;

      if (buffer.byteLength > outputSize) {
        throw new Error("file too big");
        return;
      }

      const t1 = performance.now();
      const ptr = Module._malloc(buffer.byteLength);
      Module
        .HEAPU8
        .set(new Uint8Array(buffer), ptr);
      let usedSize = Module._process(ptr, buffer.byteLength);
      console.log('remux take:', performance.now() - t1);
      if (usedSize < 0) {
        //error
        self.postMessage({type: "error", data: "remux error"})
        return;
      }
      let result = Module
        .HEAPU8
        .subarray(outPtr, outPtr + usedSize);
      let mp4 = new Uint8Array(result);
      self.postMessage({
        type: "mp4",
        data: mp4
      }, [mp4.buffer]);
      Module._free(ptr);
      break;

    case "free":
      // free when no use
      Module._free(outPtr);
      console.log('free out buffer');
      break;
  }
})