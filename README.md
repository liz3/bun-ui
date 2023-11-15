# BUN UI
This is a library for rendering graphs, plots and pie charts in javascript using [bun](https://bun.sh)

It provides a high level api for generating graphs, plots and pies and optionally display them as window.


## Explanation
This uses bun FFI api to create GLFW windows using a RGBA shader which can render buffers from a cpu, this is not to provide a graphics api but the interface to render them onto the screen
with a simple api. See lib/index.mjs

## Installation
Add this as a git based dependency.

## API

### Graph
```js
import {graph} from "bun-ui";
// graph = (name:string, graphs: [[]number], markers: ?[][n:number, name: string], options: ?{width: number, height: number}): {canvas:Canvas, w: number, h: number}
const {canvas, w, h} = graph("Title", [[0.4, 0.2, 0.5, 0.1]], [[0, "0"], [1, "100"]]);
```
### Plot
Colors is rgb 0-255
```js
import {plot} from "bun-ui";
// plot = (name:string, bars: [number|[number, string]], markers: ?[][n:number, name: string], color: [number, number, number], options: ?{width: number, height: number}): {canvas:Canvas, w: number, h: number}
const {canvas, w, h} = plot("Title", [0.4, 0.2, 0.5, [0.1, "10%"]], [[0, "0"], [1, "100"]]);
```
### Pie
Note that the addition of the values needs to end at 1
```js
import {pie} from "bun-ui";
// pie = (name:string, parts: [number|[number, string]], options: ?{width: number, height: number}): {canvas:Canvas, w: number, h: number}
const {canvas, w, h} = pie("Title", [[0.4, "40%"], [0.6, "60%"]]);
```
### Automap
Normalizes a range of values for plots
```js
import {autoMap, plot} from "bun-ui";
// autoMap = (values: []number): [[]numbers, [[number, string]]]
const values = [3456,6345,2345,5756];
const [normalized, markers] = autoMap(values);
const {canvas, w, h } = plot("Plot", normalized, markers);

```
### toWindow
Display a window based on a render, the promise resolves when the window is closed
```js
import {toWindow, plot} from "bun-ui";
// toWindow = (title:string, in: {canvas: Canvas, w: number, h: number}): Promise<void>
const p = plot("Plot Title", [0.4, 0.2, 0.5, [0.1, "10%"]], [[0, "0"], [1, "100"]]);
await toWindow("Window Title", p);
```
### iterativeWindow
Display a window based on a render and a index, this is a "easy way" to update a exiting buffer of a window based on an index
```js
import {iterativeWindow, plot} from "bun-ui";
// iterativeCallback = (index: number): {buffer: Buffer, w: number, h: number, index: ?number, type: ?"rgb"|"rgba"|"bgra"}
// iterativeWindow = (windowTitle:string, w: number, h: number, callback: iterativeCallback, initial_index: ?number = 1): Promise<void>

await iterativeWindow(
    "Window title",
    400,
    400,
    (givenIndex) => {
        const { canvas, w, h } = plot(
            "Plot Title",
            [1 / givenIndex],
            [
                [0, "0"],
                [1, "100"],
            ],
            [0, 50, 200],
            {
                width: 400,
                height: 400,
            },
        );
        return { buffer: canvas.toBuffer("raw"), w, h };
    },
    1,
);
```
### Window
Window is the underlying class on which all apis build upon, it gives you very low level control.
```js
import Window, {plot} from "bun-ui";
/* 
class Window {
    constructor(windowTitle:string, windowWidth: number, windowHeight: number);
    setCloseCallback((): void): void;
    close(): void;
    setClearColor(red: number, green: number, blue: number): void;
    updateBuffer(buffer: Buffer, bufferWidth: number, bufferHeight: number, type: ?"rgb"|"rgba"|"bgra" = "rgba"): void;
    setKeyCallback(({key: number, scancode: number, action: number, mods: number}):void):void;
    setTextCallback((codepoint: number):void):void;
    updateTitle(title:string):void;
    create():void;
}
*/
const window = new Window("Window title", 600, 400);
window.create();
window.setClearColor(200, 200, 200);
const { canvas, w, h } = plot(
    "Plot Title",
    [1 / givenIndex],
    [
        [0, "0"],
        [1, "100"],
    ],
    [0, 50, 200],
    {
        width: 400,
        height: 400,
    },
);
window.updateBuffer(canvas.toBuffer("raw"), w, h, "bgra");
window.setTextCallback(codepoint => {
    console.log(codepoint);
});

// do something else
window.close();
```

## Building
You need CMake and a compiler which can compile C11.
Then building is pretty easy.
```
> mkdir build
> cd build
> cmake ..
> make
```

## Example of a single window
```js
import {easyWindow} from "bun-ui"

const b = Buffer.alloc(400 * 400 * 4);
for(let i = 0; i < b.length; i++) {
    b[i] = 120;
}
await easyWindow("Test Window", b, 400, 400);
``` 
<img width="962" alt="image" src="https://github.com/liz3/bun-ui/assets/21298625/04d430de-2466-4df8-bc41-61c98d08cc6e">
