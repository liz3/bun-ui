import { dlopen, FFIType, suffix, ptr, JSCallback } from "bun:ffi";
import { join } from "path";
const path = join(
    import.meta.dir,
    "..",
    "build",
    process.platform === "darwin" ? "libbun-ui.dylib" : "libbun-ui.so",
);

const lib = dlopen(path, {
    create_window: {
        args: [
            FFIType.cstring,
            FFIType.u64,
            FFIType.u64,
            FFIType.u64,
            FFIType.u64,
            FFIType.callback,
        ],
        returns: FFIType.ptr,
    },
    render_window: {
        args: [FFIType.ptr],
        returns: FFIType.u8,
    },
    move_buffer_to_image: {
        args: [FFIType.ptr, FFIType.ptr, FFIType.u32, FFIType.u32],
        returns: FFIType.u8,
    },
    dispose_instance: {
        args: [FFIType.ptr],
        returns: FFIType.u8,
    },
    set_buffer_color_type: {
        args: [FFIType.ptr, FFIType.cstring],
        returns: FFIType.u8,
    },
    update_title: {
        args: [FFIType.ptr, FFIType.cstring],
        returns: FFIType.u8,
    },
});

class Window {
    constructor(title, w, h) {
        this.title = title;
        this.w = w;
        this.h = w;
        this.created = false;
        this.interval = null;
        this.should_close = false;
        this.tick_interval = 50;
        this.color_type = "rgba";
    }
    setCloseCallback(cb) {
        this.close_calle = cb;
    }
    close() {
        if (!this.created) return;
        clearInterval(this.interval);
        this.interval = null;
        lib.symbols.dispose_instance(this.instance);
        this.instance = null;
        this.closeCallback.close();
        this.created = false;
        if (this.close_calle) this.close_calle();
    }
    updateBuffer(buffer, w, h, type = "rgba") {
        if (!this.created) return;
        if (type !== this.color_type) {
            const lower = type.toLowerCase();
            if (lower === "rgb" || lower === "rgba" || lower === "bgra") {
                lib.symbols.set_buffer_color_type(
                    this.instance,
                    Buffer.from(lower + "\0", "utf-8"),
                );
                this.color_type = lower;
            } else {
                return;
            }
        }
        lib.symbols.move_buffer_to_image(this.instance, ptr(buffer), w, h);
        this.force_render();
    }
    force_render() {
        if (!this.created) return;
        lib.symbols.render_window(this.instance);
    }
    updateTitle(title) {
        this.title = title;
        const name_buffer = Buffer.from(title + "\0", "utf-8");
        lib.symbols.update_title(this.instance, name_buffer);
    }
    create() {
        if (this.created) return;
        this.closeCallback = new JSCallback(
            (ptr) => {
                if (!this.created) return false;
                this.should_close = true;
                return true;
            },
            {
                returns: "u8",
                args: ["ptr"],
            },
        );
        const name_buffer = Buffer.from(this.title + "\0", "utf-8");
        this.instance = lib.symbols.create_window(
            name_buffer,
            this.w,
            this.h,
            this.w,
            this.h,
            this.closeCallback,
        );
        this.created = true;
        this.interval = setInterval(() => {
            if (this.should_close) {
                this.close();
                return;
            }
            lib.symbols.render_window(this.instance);
        }, this.tick_interval);
    }
}

export const easyWindow = (title, buffer, w, h, type = "rgba") => {
    return new Promise((resolve) => {
        const window = new Window(title, w, h);
        window.setCloseCallback(() => {
            resolve();
        });
        window.create();
        window.updateBuffer(buffer, w, h, type);
    });
};

export const easyWindowWithBounds = (title, buffer, w, h, ww, wh, type = "rgba") => {
    return new Promise((resolve) => {
        const window = new Window(title, ww, wh);
        window.setCloseCallback(() => {
            resolve();
        });
        window.create();
        window.updateBuffer(buffer, w, h, type);
    });
};


export default Window;
