import {easyWindowWithBounds} from "../lib/index.mjs"

const w_width = 1024;
const w_height = 1024;
const buffer_w = 2048;
const buffer_h = 2048;

const buffer = Buffer.alloc(buffer_w * buffer_h * 3);
for(let i = 0; i < buffer.length; i++){
    buffer[i] = 20;
}

await easyWindowWithBounds("Square test", buffer, buffer_w, buffer_h, w_width, w_height, "rgb")