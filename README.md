# BUN UI
This is a library which is a javascript glfw binding and a tool for rendering graphs, plots and pie charts in javascript using [bun](https://bun.sh) or [node](https://nodejs.org/en)(because bun fired me pretty disgustingly)


## Explanation
This uses bun or nodes FFI api to create GLFW windows using a RGBA shader which can render buffers from a cpu, it further provides a api for various interactions with the window and functions to render plots and the likes.

## Installation
Add this as a git based dependency.

### Node
By default it only build the bun c library, to make it work for node cd into `node_modules/bun-ui` and run `npm run build-node`, which will build the required module,
from there you can use the api normally.

### Building Manually
You need CMake and a compiler which can compile C11.
Then building is pretty easy.
```
> mkdir build
> cd build
> cmake ..
> make
```

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
### toPNG
Export a render to a PNG and save it to the filesystem.
```js
import {toPNG, plot} from "bun-ui";
// toPNG = (path:string, in: {canvas: Canvas}, background: ?string = "rgb(200, 200, 200)"): Promise<void>
const p = plot("Plot Title", [0.4, 0.2, 0.5, [0.1, "10%"]], [[0, "0"], [1, "100"]]);
await toPNG("foo/bar/out.png", p);
```
### toJPEG
Export a render to a JPEG and save it to the filesystem.
```js
import {toJPEG, plot} from "bun-ui";
// toJPEG = (path:string, in: {canvas: Canvas}, quality: ?Number = 0.95): Promise<void>
const p = plot("Plot Title", [0.4, 0.2, 0.5, [0.1, "10%"]], [[0, "0"], [1, "100"]]);
await toJPEG("foo/bar/out.png", p);
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

### easyWindowWithBounds
Easy wrapper for creating a single buffer window, `toWindow` uses this under the hood.
```js
import {easyWindowWithBounds} from "bun-ui";
// easyWindowWithBounds: (name:string, buffer:Buffer, buffer_w: number, buffer_h: number, window_w: number, window_h: number, type: ?"rgb"|"rgba"|"bgra" = "rgba")
const { canvas, w, h } = o;
return easyWindowWithBounds(
    name,
    canvas.toBuffer("raw"),
    w,
    h,
    w,
    h,
    "bgra",
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
    setMousePositionCallback((x: number, y:number):void):void
    setMouseButtonCallback((button: number, action: number, mods: number):void):void
    setFocusCallback((window_focused:boolean):void):void
    setSizeCallback((width: number, height: number, xScale: number, yScale: number):void):void
    updateTitle(title:string):void;
    getClipboard():string;
    setClipboard(content: string): void;
    dangerouslyAwaitEvents():void // this will call into glfw blocking the main thread since the ui is executed on the main js thread.
    dangerouslyAwaitEventsTimeout(seconds: number):void // this will call into glfw blocking the main thread since the ui is executed on the main js thread.

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

# License
This is free software under GPL 2.0