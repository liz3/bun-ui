import { iterativeWindow } from "../lib/index.mjs";
import {createCanvas} from "canvas";

await iterativeWindow("test", 400, 400, (index) => {
    const canvas = createCanvas(400, 400);
    const ctx = canvas.getContext("2d");
    ctx.font = "bold 13px Arial";
    ctx.fillStyle = `rgb(50, 50, 50)`;
    const {width} = ctx.measureText(`Index: ${index}`)
    ctx.fillText(`Index: ${index}`, 200 - width /2, 200-7);

    return {
        buffer: canvas.toBuffer("raw"),
        w: 400,
        h: 400,
    }
});
