#include <cstddef>
#include <napi.h>
#include <iostream>
#include <map>
#include <stdint.h>
#include <vector>
#include "bun-ui.h"

struct NodeState {
  std::map<size_t, UiInstance *> instances;
  std::unordered_map<UiInstance *, Napi::FunctionReference> keyboardCallbacks;
  std::unordered_map<UiInstance *, Napi::FunctionReference>
      windowFocusCallbacks;
  std::unordered_map<UiInstance *, Napi::FunctionReference>
      windowCloseCallbacks;
  std::unordered_map<UiInstance *, Napi::FunctionReference> textCallbacks;
  std::unordered_map<UiInstance *, Napi::FunctionReference>
      mousePositionCallbacks;
  std::unordered_map<UiInstance *, Napi::FunctionReference>
      mouseButtonCallbacks;
  std::unordered_map<UiInstance *, Napi::FunctionReference>
      frameBufferCallbacks;
  size_t idx = 0;
};
NodeState *node_state_g = nullptr;

size_t getIndexFromInstance(UiInstance *instance) {
  for (auto &[key, value] : node_state_g->instances) {
    if (value == instance)
      return key;
  }
  return 0;
}

uint8_t naa_close_cb(UiInstance *instance) {

  auto &func = node_state_g->windowCloseCallbacks[instance];
  auto env = func.Env();
  func.Call({Napi::Number::New(env, getIndexFromInstance(instance))});
  return 0;
}

uint8_t naa_key_cb(UiInstance *instance, int32_t key, int32_t scancode,
                   int32_t action, int32_t mods) {
  auto &func = node_state_g->keyboardCallbacks[instance];
  auto env = func.Env();
  func.Call({Napi::Number::New(env, getIndexFromInstance(instance)),
             Napi::Number::New(env, key), Napi::Number::New(env, scancode),
             Napi::Number::New(env, action), Napi::Number::New(env, mods)});
  return 0;
}
uint8_t naa_text_cb(UiInstance *instance, uint32_t cp) {

  auto &func = node_state_g->textCallbacks[instance];
  auto env = func.Env();
  func.Call({Napi::Number::New(env, getIndexFromInstance(instance)),
             Napi::Number::New(env, cp)});
  return 0;
}

uint8_t naa_window_focus_cb(UiInstance *instance, uint32_t cp) {

  auto &func = node_state_g->windowFocusCallbacks[instance];
  auto env = func.Env();
  func.Call({Napi::Number::New(env, getIndexFromInstance(instance)),
             Napi::Number::New(env, cp)});
  return 0;
}

uint8_t naa_fb_cb(UiInstance *instance, int32_t w, int32_t h, float xscale,
                  float yscale) {
  auto &func = node_state_g->frameBufferCallbacks[instance];
  auto env = func.Env();
  func.Call({Napi::Number::New(env, getIndexFromInstance(instance)),
             Napi::Number::New(env, w), Napi::Number::New(env, h),
             Napi::Number::New(env, xscale), Napi::Number::New(env, yscale)});
  return 0;
}
uint8_t naa_mouse_pos_cb(UiInstance *instance, double x, double y) {

  auto &func = node_state_g->mousePositionCallbacks[instance];
  auto env = func.Env();
  func.Call({Napi::Number::New(env, getIndexFromInstance(instance)),
             Napi::Number::New(env, x), Napi::Number::New(env, y)});
  return 0;
}

uint8_t naa_mouse_button_cb(UiInstance *instance, int32_t button,
                            int32_t action, int32_t mods) {
  auto &func = node_state_g->mouseButtonCallbacks[instance];
  auto env = func.Env();
  func.Call({Napi::Number::New(env, getIndexFromInstance(instance)),
             Napi::Number::New(env, button), Napi::Number::New(env, action),
             Napi::Number::New(env, mods)});
  return 0;
}
void push_callback(
    std::unordered_map<UiInstance *, Napi::FunctionReference> &map,
    UiInstance *instance, Napi::Function &func) {
  auto ref = Napi::Persistent(func);
  if (map.count(instance))
    map[instance].Unref();
  map[instance] = std::move(ref);
}

Napi::Value CreateInstance(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 6) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  std::string title = info[0].As<Napi::String>();
  int32_t buffer_w = info[1].As<Napi::Number>();
  int32_t buffer_h = info[2].As<Napi::Number>();
  int32_t window_width = info[3].As<Napi::Number>();
  int32_t window_height = info[4].As<Napi::Number>();
  Napi::Function closeCallback = info[5].As<Napi::Function>();
  std::string title_str = title;
  UiInstance *instance =
      create_window((char *)title_str.c_str(), buffer_w, buffer_h, window_width,
                    window_height, (void *)&naa_close_cb);
  if (!instance) {
    // todo handle
    return env.Null();
  }
  push_callback(node_state_g->windowCloseCallbacks, instance, closeCallback);
  auto index = node_state_g->idx++;
  node_state_g->instances[index] = instance;
  return Napi::Number::New(env, (double)index);
}

Napi::Value RenderWindow(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  render_window(instances[index]);

  return Napi::Number::New(env, 0);
}
Napi::Value MoveBufferToImage(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 4) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  Napi::Buffer<uint8_t> buffer = info[1].As<Napi::Buffer<uint8_t>>();
  int32_t buffer_w = info[2].As<Napi::Number>();
  int32_t buffer_h = info[3].As<Napi::Number>();
  move_buffer_to_image(instances[index], buffer.Data(), buffer_w, buffer_h);
  return Napi::Number::New(env, 0);
}
Napi::Value DisposeInstance(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();

  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  UiInstance *instance = instances[index];
  std::vector<std::unordered_map<UiInstance *, Napi::FunctionReference> *>
      maps = {&(node_state_g->keyboardCallbacks),
              &node_state_g->frameBufferCallbacks,
              &node_state_g->mouseButtonCallbacks,
              &node_state_g->mousePositionCallbacks,
              &node_state_g->textCallbacks,
              &node_state_g->windowFocusCallbacks,
              &node_state_g->windowCloseCallbacks};
  for (auto *e : maps) {
    if (e->count(instance)) {
      (*e)[instance].Unref();
      e->erase(instance);
    }
  }
  dispose_instance(instance);
  instances.erase(index);
  return Napi::Number::New(env, 0);
}

Napi::Value UpdateTitle(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  std::string title = info[1].As<Napi::String>();
  update_title(instances[index], title.c_str());
  return Napi::Number::New(env, 0);
}
Napi::Value SetBufferColorType(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  std::string title = info[1].As<Napi::String>();
  set_buffer_color_type(instances[index], title.c_str());
  return Napi::Number::New(env, 0);
}
Napi::Value SetClearColor(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 4) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  int32_t r = info[1].As<Napi::Number>();
  int32_t g = info[2].As<Napi::Number>();
  int32_t b = info[3].As<Napi::Number>();
  set_clear_color(instances[index], r, g, b);
  return Napi::Number::New(env, 0);
}

Napi::Value SetKeyboardCallback(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  Napi::Function callback = info[1].As<Napi::Function>();
  push_callback(node_state_g->keyboardCallbacks, instances[index], callback);
  set_keyboard_callback(instances[index], (void *)&naa_key_cb);
  return Napi::Number::New(env, 0);
}

Napi::Value SetTextCallback(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  Napi::Function callback = info[1].As<Napi::Function>();
  push_callback(node_state_g->textCallbacks, instances[index], callback);
  set_text_callback(instances[index], (void *)&naa_text_cb);
  return Napi::Number::New(env, 0);
}
Napi::Value SetFrameBufferCallback(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  Napi::Function callback = info[1].As<Napi::Function>();
  push_callback(node_state_g->frameBufferCallbacks, instances[index], callback);
  set_text_callback(instances[index], (void *)&naa_text_cb);
  return Napi::Number::New(env, 0);
}
Napi::Value SetMousePositionCallback(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  Napi::Function callback = info[1].As<Napi::Function>();
  push_callback(node_state_g->mousePositionCallbacks, instances[index],
                callback);
  set_mouse_position_callback(instances[index], (void *)&naa_mouse_pos_cb);
  return Napi::Number::New(env, 0);
}

Napi::Value SetMouseButtonCallback(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  Napi::Function callback = info[1].As<Napi::Function>();
  push_callback(node_state_g->mouseButtonCallbacks, instances[index], callback);
  set_mouse_button_callback(instances[index], (void *)&naa_mouse_button_cb);
  return Napi::Number::New(env, 0);
}

Napi::Value SetWindowFocusCallback(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  Napi::Function callback = info[1].As<Napi::Function>();
  push_callback(node_state_g->windowFocusCallbacks, instances[index], callback);
  set_window_focus_callback(instances[index], (void *)&naa_window_focus_cb);
  return Napi::Number::New(env, 0);
}

Napi::Value AwaitEvents(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  await_events(instances[index]);
  return Napi::Number::New(env, 0);
}

Napi::Value AwaitEventsTimeout(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);

  double tm = info[1].As<Napi::Number>();
  await_events_timeout(instances[index], tm);
  return Napi::Number::New(env, 0);
}

Napi::Value SetIsManaged(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);

  uint32_t v = info[1].As<Napi::Number>();
  set_is_managed(instances[index], v);
  return Napi::Number::New(env, 0);
}

Napi::Value GetClipboard(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 1) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  const char *content = get_clipboard(instances[index]);
  return Napi::String::New(env, content);
}

Napi::Value SetClipboard(const Napi::CallbackInfo &info) {
  Napi::Env env = info.Env();
  if (info.Length() < 2) {
    Napi::TypeError::New(env, "Wrong number of arguments")
        .ThrowAsJavaScriptException();
    return env.Null();
  }
  int32_t index = info[0].As<Napi::Number>();
  auto &instances = node_state_g->instances;
  if (!instances.count(index))
    return Napi::Number::New(env, 1);
  std::string content = info[1].As<Napi::String>();
  set_clipboard(instances[index], content.c_str());
  return env.Null();
}

static Napi::Object Init(Napi::Env env, Napi::Object exports) {
  if (node_state_g == nullptr)
    node_state_g = new NodeState();
  exports.Set(Napi::String::New(env, "create_window"),
              Napi::Function::New(env, CreateInstance));
  exports.Set(Napi::String::New(env, "render_window"),
              Napi::Function::New(env, RenderWindow));
  exports.Set(Napi::String::New(env, "move_buffer_to_image"),
              Napi::Function::New(env, MoveBufferToImage));
  exports.Set(Napi::String::New(env, "dispose_instance"),
              Napi::Function::New(env, DisposeInstance));
  exports.Set(Napi::String::New(env, "update_title"),
              Napi::Function::New(env, UpdateTitle));
  exports.Set(Napi::String::New(env, "set_buffer_color_type"),
              Napi::Function::New(env, SetBufferColorType));
  exports.Set(Napi::String::New(env, "set_clear_color"),
              Napi::Function::New(env, SetClearColor));
  exports.Set(Napi::String::New(env, "set_keyboard_callback"),
              Napi::Function::New(env, SetKeyboardCallback));
  exports.Set(Napi::String::New(env, "set_text_callback"),
              Napi::Function::New(env, SetTextCallback));
  exports.Set(Napi::String::New(env, "set_framebuffer_callback"),
              Napi::Function::New(env, SetFrameBufferCallback));
  exports.Set(Napi::String::New(env, "set_mouse_position_callback"),
              Napi::Function::New(env, SetMousePositionCallback));
  exports.Set(Napi::String::New(env, "set_mouse_button_callback"),
              Napi::Function::New(env, SetMouseButtonCallback));
  exports.Set(Napi::String::New(env, "set_window_focus_callback"),
              Napi::Function::New(env, SetWindowFocusCallback));
  exports.Set(Napi::String::New(env, "await_events"),
              Napi::Function::New(env, AwaitEvents));
  exports.Set(Napi::String::New(env, "await_events_timeout"),
              Napi::Function::New(env, AwaitEventsTimeout));
  exports.Set(Napi::String::New(env, "set_is_managed"),
              Napi::Function::New(env, SetIsManaged));
  exports.Set(Napi::String::New(env, "get_clipboard"),
              Napi::Function::New(env, GetClipboard));
  return exports;
}

NODE_API_MODULE(BunUi, Init)