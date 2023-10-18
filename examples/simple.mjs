import {easyWindow} from "../lib/index.mjs"

console.log("run")

const b = Buffer.alloc(400 * 400 * 3);
for(let i = 0; i < b.length; i++) {
    b[i] = 120;
}
await easyWindow("Test hehe", b, 400, 400, "rgb");
console.log("done")