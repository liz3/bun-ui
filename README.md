# BUN UI
This is a proof of concept binding between bun and glfw for rendering RGB or RGBA buffers into simple windows.

## Explanation
This uses bun FFI api to create GLFW windows using a RGBA shader which can render buffers from a cpu, this is not to provide a graphics api but the interface to render them onto the screen
with a simple api. See lib/index.mjs

## Building
You need CMake and a compiler which can compile C11.
Then building is pretty easy.
```
> git submodule update --init
> mkdir build
> cd build
> cmake ..
> make
```

## Example
```js
import {easyWindow} from "../lib/index.mjs"

const b = Buffer.alloc(400 * 400 * 4);
for(let i = 0; i < b.length; i++) {
    b[i] = 120;
}
await easyWindow("Test Window", b, 400, 400);
``` 