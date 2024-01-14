import { join, dirname } from "path";
import { fileURLToPath } from "url";
import { createCanvas } from "canvas";
let bunImports = null;
const isBun = process.versions.bun !== undefined;

if (isBun) {
  bunImports = await import("bun:ffi");
} else {
  bunImports = {
    dlopen: async (path) => {
      const res = await import("bindings");
      return {
        symbols: res.default(path),
      };
    },
    FFIType: {},
    ptr: (i) => i,
  };
}
const { dlopen, FFIType, ptr, JSCallback: jsCallbackConstructor } = bunImports;
const JSCallback = (func, args) => {
  if (isBun) return new jsCallbackConstructor(func, args);
  func.close = () => {};
  return func;
};

const path = isBun
  ? join(
      import.meta.dir,
      "..",
      "build",
      process.platform === "darwin" ? "libbun-ui.dylib" : "libbun-ui.so",
    )
  : "bun-ui.node";
const lib = await dlopen(path, {
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
  set_clear_color: {
    args: [FFIType.ptr, FFIType.u8, FFIType.u8, FFIType.u8],
    returns: FFIType.u8,
  },
  set_keyboard_callback: {
    args: [FFIType.ptr, FFIType.callback],
    returns: FFIType.u8,
  },
  set_text_callback: {
    args: [FFIType.ptr, FFIType.callback],
    returns: FFIType.u8,
  },
  set_framebuffer_callback: {
    args: [FFIType.ptr, FFIType.callback],
    returns: FFIType.u8,
  },
  set_mouse_position_callback: {
    args: [FFIType.ptr, FFIType.callback],
    returns: FFIType.u8,
  },
  set_mouse_button_callback: {
    args: [FFIType.ptr, FFIType.callback],
    returns: FFIType.u8,
  },
  set_window_focus_callback: {
    args: [FFIType.ptr, FFIType.callback],
    returns: FFIType.u8,
  },
  await_events: {
    args: [FFIType.ptr],
    returns: FFIType.u8,
  },
  await_events_timeout: {
    args: [FFIType.ptr, FFIType.f64],
    returns: FFIType.u8,
  },
  set_is_managed: {
    args: [FFIType.ptr, FFIType.u8],
    returns: FFIType.u8,
  },
  get_clipboard: {
    args: [FFIType.ptr],
    returns: FFIType.cstring,
  },
  set_clipboard: {
    args: [FFIType.ptr, FFIType.cstring],
    returns: FFIType.u8,
  },
});

class Window {
  constructor(title, w, h, managed = true) {
    this.title = title;
    this.w = w;
    this.h = h;
    this.managed = managed;
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
    if (this.interval) clearInterval(this.interval);
    this.interval = null;
    lib.symbols.dispose_instance(this.instance);
    this.instance = null;
    this.closeCallback.close();
    if (this.internalKeyCallback) this.internalKeyCallback.close();
    if (this.internalTextCallback) this.internalTextCallback.close();
    if (this.internalSizeCallback) this.internalSizeCallback.close();
    if (this.internalMousePositionCallback)
      this.internalMousePositionCallback.close();
    if (this.internalMouseButtonCallback)
      this.internalMouseButtonCallback.close();
    if (this.internalWindowFocusCallback)
      this.internalWindowFocusCallback.close();
    this.created = false;
    if (this.close_calle) this.close_calle();
  }
  setClearColor(r, g, b) {
    if (!this.created) return;
    lib.symbols.set_clear_color(this.instance, r, g, b);
  }
  updateBuffer(buffer, w, h, type = "rgba") {
    if (!this.created) return;
    if (type !== this.color_type) {
      const lower = type.toLowerCase();
      if (lower === "rgb" || lower === "rgba" || lower === "bgra") {
        lib.symbols.set_buffer_color_type(
          this.instance,
          isBun ? Buffer.from(lower + "\0", "utf-8") : lower,
        );
        this.color_type = lower;
      } else {
        return;
      }
    }
    lib.symbols.move_buffer_to_image(this.instance, ptr(buffer), w, h);
    this.force_render();
  }

  setKeyCallback(cb) {
    if (this.keyCallback || !this.created) return;
    this.keyCallback = cb;
    this.internalKeyCallback = JSCallback(
      (instance, key, scancode, action, mods) => {
        this.keyCallback({ key, scancode, action, mods });
        return true;
      },
      {
        args: ["ptr", "i32", "i32", "i32", "i32"],
        returns: "u8",
      },
    );
    lib.symbols.set_keyboard_callback(this.instance, this.internalKeyCallback);
  }
  dangerouslyAwaitEvents() {
    if (!this.created || this.managed) return;
    lib.symbols.await_events(this.instance);
  }

  dangerouslyAwaitEventsTimeout(m) {
    if (!this.created || this.managed) return;
    lib.symbols.await_events_timeout(this.instance, m);
  }

  setTextCallback(cb) {
    if (this.textCallback || !this.created) return;
    this.textCallback = cb;
    this.internalTextCallback = JSCallback(
      (instance, cp) => {
        this.textCallback(cp);
        return true;
      },
      {
        args: ["ptr", "u32"],
        returns: "u8",
      },
    );
    lib.symbols.set_text_callback(this.instance, this.internalTextCallback);
  }

  getClipboard() {
    if (!this.created) return;
    return lib.symbols.get_clipboard(this.instance);
  }
  setClipboard(str) {
    if (!this.created) return;
    const b = isBun ? Buffer.from(str + "\0", "utf-8") : str;
    lib.symbols.set_clipboard(this.instance, b);
  }

  setSizeCallback(cb) {
    if (this.sizeCallback || !this.created) return;
    this.sizeCallback = cb;
    this.internalSizeCallback = JSCallback(
      (instance, w, h, x, y) => {
        cb(w, h, x, y);
        return true;
      },
      {
        args: ["ptr", "i32", "i32", "f32", "f32"],
        returns: "u8",
      },
    );
    lib.symbols.set_framebuffer_callback(
      this.instance,
      this.internalSizeCallback,
    );
  }

  setMousePositionCallback(cb) {
    if (this.mousePositionCallback || !this.created) return;
    this.mousePositionCallback = cb;
    this.internalMousePositionCallback = new JSCallback(
      (instance, x, y) => {
        cb(x, y);
        return true;
      },
      {
        args: ["ptr", "f64", "f64"],
        returns: "u8",
      },
    );
    lib.symbols.set_mouse_position_callback(
      this.instance,
      this.internalMousePositionCallback,
    );
  }

  setMouseButtonCallback(cb) {
    if (this.mouseButtonCallback || !this.created) return;
    this.mouseButtonCallback = cb;
    this.internalMouseButtonCallback = JSCallback(
      (instance, button, action, mods) => {
        cb({ button, action, mods });
        return true;
      },
      {
        args: ["ptr", "i32", "i32", "i32"],
        returns: "u8",
      },
    );
    lib.symbols.set_mouse_button_callback(
      this.instance,
      this.internalMouseButtonCallback,
    );
  }

  setFocusCallback(cb) {
    if (this.windowFocusCallback || !this.created) return;
    this.windowFocusCallback = cb;
    this.internalWindowFocusCallback = JSCallback(
      (instance, f) => {
        cb(f === 1 ? true : false);
        return true;
      },
      {
        args: ["ptr", "i32"],
        returns: "u8",
      },
    );
    lib.symbols.set_window_focus_callback(
      this.instance,
      this.internalWindowFocusCallback,
    );
  }

  force_render() {
    if (!this.created) return;
    lib.symbols.render_window(this.instance);
  }
  updateTitle(title) {
    this.title = title;
    const name_buffer = isBun ? Buffer.from(title + "\0", "utf-8") : title;
    lib.symbols.update_title(this.instance, name_buffer);
  }
  setCloseRequestCallback(cb) {
    this.closeRequestCallback = cb;
  }
  create() {
    if (this.created) return;
    this.closeCallback = JSCallback(
      (ptr) => {
        if (!this.managed) {
          if (this.closeRequestCallback) this.closeRequestCallback();
          return;
        }
        if (!this.created) return false;
        this.should_close = true;
        return true;
      },
      {
        returns: "u8",
        args: ["ptr"],
      },
    );
    const name_buffer = isBun
      ? Buffer.from(this.title + "\0", "utf-8")
      : this.title;
    this.instance = lib.symbols.create_window(
      name_buffer,
      this.w,
      this.h,
      this.w,
      this.h,
      this.closeCallback,
    );
    this.created = true;
    if (this.managed) {
      this.interval = setInterval(() => {
        if (this.should_close) {
          this.close();
          return;
        }
        lib.symbols.render_window(this.instance);
      }, this.tick_interval);
    } else {
      lib.symbols.set_is_managed(this.instance, 0);
    }
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

export const easyWindowWithBounds = (
  title,
  buffer,
  w,
  h,
  ww,
  wh,
  type = "rgba",
) => {
  return new Promise((resolve) => {
    const window = new Window(title, ww, wh);
    window.setCloseCallback(() => {
      resolve();
    });
    window.create();
    window.setClearColor(200, 200, 200);
    window.updateBuffer(buffer, w, h, type);
  });
};
const getRandomColor = () => {
  var letters = "0123456789ABCDEF";
  var color = "#";
  for (var i = 0; i < 6; i++) {
    color += letters[Math.floor(Math.random() * 16)];
  }
  return color;
};
export const plot = (
  name,
  entries,
  markings,
  color = [0, 50, 200],
  options = {
    width: 650,
    height: 500,
  },
) => {
  const { width: w, height: h } = options;
  const canvas = createCanvas(w, h);
  const ctx = canvas.getContext("2d");
  const work_height = h * 0.9;
  let l_offset = 0;
  for (const marking of markings) {
    const height_needed = work_height * marking[0];
    const start_y = work_height - height_needed;
    ctx.font = "14px Arial";
    ctx.fillStyle = `rgb(50, 50, 50)`;
    ctx.fillText(marking[1], 1, start_y + 14);
    ctx.lineWidth = 1;
    ctx.strokeStyle = "rgb(50,50,50)";
    ctx.beginPath();
    ctx.moveTo(1, start_y + 1);
    ctx.lineTo(w, start_y + 1);
    ctx.stroke();
    const { width } = ctx.measureText(marking[1]);
    if (width > 20 && width - 20 > l_offset) l_offset = width - 20;
  }
  const work_width = w - (40 + l_offset);
  const text_height = h * 0.1;
  const row_width = Math.min(work_width / entries.length, 60);
  const total_needed = row_width * entries.length;
  let x_offset = (w - total_needed) / 2;
  if (l_offset > 0) x_offset += 20;
  for (const v of entries) {
    ctx.fillStyle = `rgb(${color[0]}, ${color[1]}, ${color[2]})`;
    const entry = Array.isArray(v) ? v[0] : v;
    const row_padding = row_width * 0.15;
    const entry_width = row_width - row_padding;
    const height_needed = work_height * entry;
    const start_y = work_height - height_needed;
    ctx.fillRect(
      x_offset + row_padding / 2,
      start_y,
      entry_width,
      height_needed,
    );

    if (Array.isArray(v) && typeof v[1] === "string") {
      const name = v[1];

      ctx.font = "12px Arial";
      ctx.fillStyle = `rgb(50, 50, 50)`;
      ctx.fillText(name, x_offset + row_padding / 2, work_height + 2 + 12);
    }
    x_offset += row_width;
  }

  ctx.font = "14px Arial";
  ctx.fillStyle = `rgb(50, 50, 50)`;
  let text_len = ctx.measureText(name);
  const start = w / 2 - text_len.width / 2;
  ctx.fillText(name, start, work_height + 30);
  return { canvas, w, h };
};
export const graph = (
  name,
  entries,
  markings,
  options = {
    width: 650,
    height: 250,
    markers: [],
  },
) => {
  const { width: w, height: h, markers } = options;
  const work_width = w * 0.9;
  const work_height = h * 0.7;
  const padding = (w - work_width) / 2;
  const canvas = createCanvas(w, h);
  const ctx = canvas.getContext("2d");
  const top_offset = 15;
  let name_offset = padding;

  // horizontal lines
  if (Array.isArray(markings) && markings.length > 0) {
    for (const mark of markings) {
      const [p, name] = mark;
      const start_y = work_height - work_height * p + top_offset;
      if (typeof name === "string") {
        ctx.font = "14px Arial";
        ctx.fillStyle = `rgb(50, 50, 50)`;
        ctx.fillText(name, 5, start_y);
      }
      ctx.lineWidth = 1;
      ctx.strokeStyle = "rgba(50,50,50, 0.8)";
      ctx.beginPath();
      ctx.moveTo(padding, start_y);
      ctx.lineTo(padding + work_width, start_y);
      ctx.stroke();
    }
  }
  // vertical lines
  if (markers.length) {
    const marker_len =
      work_width / (markers.length === 1 ? 1 : markers.length - 1);
    let lOffset = offset;

    ctx.strokeStyle = "rgba(50,50,50, 0.8)";
    for (const marker of markers) {
      ctx.moveTo(lOffset, top_offset);
      ctx.lineTo(lOffset, top_offset + work_height);
      ctx.fillText(marker, lOffset, top_offset + work_height - 14);
      lOffset += marker_len;
    }
  }
  for (const graph of entries) {
    ctx.lineWidth = 3;
    const color = getRandomColor();
    ctx.strokeStyle = color;
    const points = Array.isArray(graph[0]) ? graph[0] : graph;
    const point_len = work_width / (points.length - 1);
    let offset = padding;
    ctx.beginPath();

    if (points.length === 1) {
      const point = points[0];
      ctx.moveTo(offset, work_height - work_height * point + top_offset);
      ctx.lineTo(
        work_width + offset,
        work_height - work_height * point + top_offset,
      );
      ctx.stroke();
    } else {
      {
        ctx.moveTo(offset, work_height - work_height * points[0] + top_offset);
      }
      for (let i = 1; i < points.length; i++) {
        offset += point_len;
        ctx.lineTo(offset, work_height - work_height * points[i] + top_offset);
      }
      ctx.stroke();
    }
    ctx.lineWidth = 10;
    ctx.beginPath();
    ctx.moveTo(name_offset, work_height + 25);
    ctx.lineTo(name_offset + 15, work_height + 25);
    ctx.stroke();
    name_offset += 20;
    // graph`s name if present
    if (Array.isArray(graph[0])) {
      const name = graph[1];
      ctx.font = "14px Arial";
      ctx.fillStyle = color;
      ctx.fillText(name, name_offset, work_height + 30);
      const text_len = ctx.measureText(name);
      name_offset += text_len.width + 10;
    }
  }
  return { canvas, w, h };
};
export const pie = (
  name,
  entries,
  options = {
    width: 650,
    height: 500,
  },
) => {
  const { width: w, height: h } = options;
  const work_height = h * 0.9;
  const canvas = createCanvas(w, h);
  const ctx = canvas.getContext("2d");

  const total = 1;

  let startAngle = 0;

  const centerX = w / 2;
  const centerY = work_height / 2;

  const radius = work_height * 0.5 - 14;

  for (const v of entries) {
    const entry = Array.isArray(v) ? v[0] : v;

    const endAngle = startAngle + (entry / total) * 2 * Math.PI;

    ctx.beginPath();
    ctx.moveTo(centerX, centerY);
    ctx.arc(centerX, centerY, radius, startAngle, endAngle);
    ctx.closePath();

    ctx.fillStyle = getRandomColor();
    ctx.fill();
    ctx.stroke();

    if (Array.isArray(v) && typeof v[1] === "string") {
      const name = v[1];
      const nameAngle = startAngle + (entry / 2 / total) * 2 * Math.PI;
      const start_x = centerX + radius * Math.cos(nameAngle);
      const start_y = centerY + radius * Math.sin(nameAngle);

      ctx.font = "bold 13px Arial";
      ctx.fillStyle = `rgb(50, 50, 50)`;
      ctx.fillText(name, start_x, start_y);
    }
    startAngle = endAngle;
  }

  ctx.font = "14px Arial";
  ctx.fillStyle = `rgb(50, 50, 50)`;
  let text_len = ctx.measureText(name);
  const start = w / 2 - text_len.width / 2;
  ctx.fillText(name, start, work_height + 30);
  return { canvas, w, h };
};

export const toWindow = async (name, o) => {
  const { canvas, w, h } = o;
  return easyWindowWithBounds(name, canvas.toBuffer("raw"), w, h, w, h, "bgra");
};

export const iterativeWindow = (
  title,
  ww,
  wh,
  buffer_callback,
  inital_index = 0,
) => {
  return new Promise((resolve) => {
    let i = inital_index;
    const window = new Window(title, ww, wh);
    window.setCloseCallback(() => {
      resolve();
    });
    window.create();
    window.setClearColor(200, 200, 200);
    const run = async (index) => {
      const ret = await buffer_callback(index);
      if (!ret) return;
      const { buffer, w, h, type, index: passed_index } = ret;
      if (!buffer) return;
      window.updateBuffer(buffer, w, h, type || "bgra");
      i = typeof passed_index === "number" ? passed_index : index;
    };
    window.setKeyCallback((data) => {
      const { key, action } = data;
      if (action == 1 || action == 2) {
        if (key == 262 || key == 78) {
          run(i + 1);
        } else if (key == 263 || key == 66) {
          run(i - 1);
        }
      }
    });

    run(inital_index);
  });
};

export const autoMap = (input) => {
  let highest = null;
  let lowest = null;
  for (const number of input) {
    if (highest === null || number > highest) highest = number;
    if (lowest == null || number < lowest) lowest = number;
  }
  const out = [];
  for (const number of input) {
    out.push(number / highest);
  }
  const lines = [
    [1, `${highest.toFixed(2)}`],
    [0, "0.00"],
  ];
  for (const n of [0.25, 0.5, 0.75]) {
    lines.push([n, `${(highest * n).toFixed(2)}`]);
  }
  return [out, lines];
};
export default Window;
