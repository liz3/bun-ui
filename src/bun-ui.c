
#include "bun-ui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

size_t g_init = 0;
size_t loaded_glad = 0;
List g_list;

int init() {
  if(g_init == 1)
    return 0;
  g_list.size = 0;
  g_list.head = NULL;
  g_list.tail = NULL;
  glfwInit();
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwSwapInterval(1);

  g_init = 1;
  return 0;
}


GLuint simple_compile_shader(GLuint type, const char* content){
  GLuint id = glCreateShader(type);
  glShaderSource(id, 1, &content, NULL);
  glCompileShader(id);

  GLint success;
  glGetShaderiv(id, GL_COMPILE_STATUS, &success);
  if(!success){
    printf("shader compile error: %s\n", content);
  }
  return id;
}
UiInstance* create_window(char* window_title, size_t buffer_w, size_t buffer_h, size_t window_width, size_t window_height, void* close_callback) {
  if(init())
    return NULL;
  UiInstance* instance = calloc(1, sizeof(UiInstance));
  instance->close_callback = close_callback;
  instance->window_width = window_width;
  instance->window_height = window_height;
  RgbaColor clear_color = {.r = 80, .g = 80,.b = 80, .a = 255};
  instance->clear_color = clear_color;
  instance->render_buffer.type = RGBA;

  instance->window = glfwCreateWindow(window_width, window_height, window_title, NULL, NULL);
  glfwMakeContextCurrent(instance->window);
  float xscale, yscale;
  glfwGetWindowContentScale(instance->window, &xscale, &yscale);
  instance->window_width *= xscale;
  instance->window_height *= yscale;
  if(loaded_glad == 0) {
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
      return NULL;
    } 
    loaded_glad = 1;

  }
  glEnable(GL_CULL_FACE);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  ShaderVar vars[2] = {{2, sizeof(SimpleShaderEntry), GL_FLOAT, (void*)offsetof(SimpleShaderEntry, pos)},
      {2, sizeof(SimpleShaderEntry), GL_FLOAT, (void*)offsetof(SimpleShaderEntry, size)}};
  instance->shader = create_shader(IMAGE_SHADER_VERT, IMAGE_SHADER_FRAG, 16, vars, 2);
  image_buffer_resize(&(instance->render_buffer), buffer_w, buffer_h);
  allocate_texture(&(instance->render_buffer));
  instance->list_entry =list_append(&g_list, instance);
  glfwPollEvents();
  return instance;
}

uint8_t set_clear_color(UiInstance* instance, uint8_t r, uint8_t g, uint8_t b) {

  RgbaColor clear_color = {.r = r, .g = g,.b = b, .a = 255};
  instance->clear_color = clear_color;
  return 0;
}

uint8_t render_window(UiInstance* instance) {

  glfwMakeContextCurrent(instance->window);
  glfwGetFramebufferSize(instance->window, &instance->window_width, &instance->window_height);
  glViewport(0, 0, instance->window_width, instance->window_height);
  shader_set2f(instance->shader, "resolution", (float)instance->window_width, (float)instance->window_height);
  RgbaColor clear_color = instance->clear_color;
  glClearColor((float)clear_color.r / 255, (float)clear_color.g / 255, (float)clear_color.b /255, (float)clear_color.a / 255);
  glClear(GL_COLOR_BUFFER_BIT);
  move_image_buffer_to_texture(&(instance->render_buffer));
  Vec2f window_size = {instance->window_width, instance->window_height};
  float t = window_size.x;
  if(window_size.y > window_size.x)
    t = window_size.y;
  if(instance->render_buffer.h > instance->render_buffer.w) {
    float scale = t / instance->render_buffer.h ;
    window_size.x = instance->render_buffer.w * scale;
    window_size.y = instance->render_buffer.h * scale;
  } else {
    float scale = t / instance->render_buffer.w;
    window_size.x = instance->render_buffer.w * scale;
    window_size.y = instance->render_buffer.h * scale;
  }
  Vec2f start_pos = {0,0};
  if(window_size.x < instance->window_width) {
    size_t diff = instance->window_width - window_size.x;
    start_pos.x += diff / 2;
  }

  if(window_size.y < instance->window_height) {
    size_t diff = instance->window_height - window_size.y;
    start_pos.y += diff / 2;
  }
  SimpleShaderEntry entry = {normalize(instance, start_pos), window_size};
  shader_use(instance->shader);
  glBindTexture(GL_TEXTURE_2D, instance->render_buffer.texture_id);
  glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(SimpleShaderEntry), &entry);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 6, 1);
  glfwSwapBuffers(instance->window);
  glfwPollEvents();
  if(glfwWindowShouldClose(instance->window)) {
    ((close_callback*)instance->close_callback)(instance);
  }
  return 0;
}

uint8_t dispose_instance(UiInstance* instance) {
  list_remove(&g_list, (ListEntry*)instance->list_entry);
  instance->list_entry = NULL;
  glfwDestroyWindow(instance->window);
  if(instance->render_buffer.texture_was_allocated)
      glDeleteTextures(1, &(instance->render_buffer.texture_id));
  if(instance->render_buffer.buffer)
    free(instance->render_buffer.buffer);

  glDeleteProgram(instance->shader->pid);
  free(instance->shader);
  free(instance);
  return 0;
}
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    ListEntry* entry = list_find_window(&g_list, window);
    if(entry == NULL)
      return;
    if(entry->instance->key_callback == NULL)
      return;
    UiInstance* instance = entry->instance;
      ((key_cb_t*)instance->key_callback)(instance, key, scancode, action, mods);
}
void character_callback(GLFWwindow* window, unsigned int codepoint)
{
    ListEntry* entry = list_find_window(&g_list, window);
    if(entry == NULL)
      return;
    if(entry->instance->text_callback == NULL)
      return;
    UiInstance* instance = entry->instance;
      ((text_cb_t*)instance->text_callback)(instance, codepoint);
}

uint8_t set_keyboard_callback(UiInstance* instance, void* callback) {
  instance->key_callback = callback;
  glfwSetKeyCallback(instance->window, key_callback);
  return 0;
}
uint8_t set_text_callback(UiInstance* instance, void* callback) {
  instance->text_callback = callback;
  glfwSetCharCallback(instance->window, character_callback);
  return 0;
}
Vec2f normalize(UiInstance* instance, Vec2f in) {
 Vec2f a = {.x = in.x - ((float)instance->window_width / 2), .y = in.y - ((float)instance->window_height/2)};
 return a;
}


uint8_t move_buffer_to_image(UiInstance* target, uint8_t* buffer, uint32_t w, uint32_t h) {
  const uint8_t pixel_size = get_buffer_pixel_size(&target->render_buffer);
  image_buffer_resize(&(target->render_buffer), w,h);
  memcpy((&target->render_buffer)->buffer, buffer, w*h*pixel_size);
  return 0;
}
void image_buffer_resize(Image* image, uint32_t w, uint32_t h) {
  const uint8_t pixel_size = get_buffer_pixel_size(image);
  if(image->buffer) {
    if(image->buffer_size == w * h * pixel_size && w == image->w && h == image->h)
      return;
    uint8_t* resized_buffer = realloc(image->buffer, w * h * pixel_size);
    image->buffer = resized_buffer;
  } else {
    image->buffer = calloc(1, w * h * pixel_size);
  }
  image->buffer_size = w * h * pixel_size;
  image->w = w;
  image->h = h;
}
uint8_t get_buffer_pixel_size(Image* in) {
  if(in->type == RGB)
    return 3;
  return 4;
}
void allocate_texture(Image* image) {
  if(image->texture_was_allocated) {
     glDeleteTextures(1, &(image->texture_id));
  }
  glGenTextures(1, &(image->texture_id));
  image->texture_was_allocated = 1;
}

void move_image_buffer_to_texture(Image* buffer) {
  if(!buffer->texture_was_allocated)
    return;
  glActiveTexture(GL_TEXTURE0); 
  glBindTexture(GL_TEXTURE_2D, buffer->texture_id);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  GLint color_t = get_type_enum(buffer, 0);
  GLint p_type = get_type_enum(buffer, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, p_type, (GLsizei)buffer->w,
               (GLsizei)buffer->h, 0, color_t, GL_UNSIGNED_BYTE, NULL);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer->w, buffer->h, color_t,
                  GL_UNSIGNED_BYTE, buffer->buffer);


}

GLint get_type_enum(Image* in, uint8_t type) {
  if(in->type == RGB) {
    return type == 1 ? GL_RGB8 : GL_RGB;
  }
  if(in->type == RGBA) {
    return type == 1 ? GL_RGBA8 : GL_RGBA;
  }
    if(in->type == BGRA) {
    return type == 1 ? GL_RGBA8 : GL_BGRA;
  }
  return 0;
}

uint8_t set_buffer_color_type(UiInstance* instance, const char* type) {
  Image* render_buffer = &instance->render_buffer;
  if(string_match(type, "rgb")) {
    render_buffer->type = RGB;
  } else if(string_match(type, "rgba")) {
    render_buffer->type = RGBA;
  } else if(string_match(type, "bgra")) {
    render_buffer->type = BGRA;
  }
  return 0;
}

uint8_t string_match(const char* lhs, const char* rhs) {
  size_t offset = 0;
  while(1) {
    if(lhs[offset] == '\0' && rhs[offset] == '\0')
      break;
    if(lhs[offset] == '\0' || rhs[offset] == '\0')
      return 0;
    if(lhs[offset] != rhs[offset])
      return 0;
    offset++;

  }

  return 1;
}

uint8_t update_title(UiInstance* instance, const char* new_title) {
  glfwSetWindowTitle(instance->window, new_title);
  return 0;
}

void shader_use(Shader* shader) {
 glUseProgram(shader->pid);
 glBindVertexArray(shader->vao);
 glBindBuffer(GL_ARRAY_BUFFER, shader->vbo);
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
  // if(active_instance && window == active_instance->window) {
  //   active_instance->window_width = width;
  //   active_instance->window_height = height;
  //    glViewport(0, 0, width, height);
  // } else {
  //   printf("missmatch\n");
  // }
}

void shader_set2f(Shader* shader, const char* name, float x, float y) {
   glUniform2f(glGetUniformLocation(shader->pid, name), x, y);
}
void shader_set4f(Shader* shader, const char* name, float x, float y, float z, float w) {
   glUniform4f(glGetUniformLocation(shader->pid, name), x, y, z, w);
}

void shader_set1f(Shader* shader, const char* name, float v) {
   glUniform1f(glGetUniformLocation(shader->pid, name), v);
}
Shader* create_shader(const char* vertex_content, const char* fragment_content, uint32_t size, ShaderVar* vars, size_t shader_var_len) {

  Shader* shader = calloc(1, sizeof(Shader));
  glGenVertexArrays(1, &(shader->vao));
  glGenBuffers(1, &(shader->vbo));
  glBindVertexArray(shader->vao);
  glBindBuffer(GL_ARRAY_BUFFER, shader->vbo);
  glBufferData(GL_ARRAY_BUFFER, size, NULL, GL_DYNAMIC_DRAW);
  int count = 0;
  for (size_t i = 0; i < shader_var_len; i++) {
    ShaderVar entry = vars[i];
    glEnableVertexAttribArray(count);
    glVertexAttribPointer(count, entry.count, entry.type, GL_FALSE, entry.size,
      entry.offset);
    glVertexAttribDivisor(count, 1);
    count++;
  }

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
  GLuint pid = glCreateProgram();
  GLuint vertex_shader = simple_compile_shader(GL_VERTEX_SHADER, vertex_content);
  GLuint fragment_shader = simple_compile_shader(GL_FRAGMENT_SHADER, fragment_content);
  glAttachShader(pid, vertex_shader);
  glAttachShader(pid, fragment_shader);
  glLinkProgram(pid);
  shader->pid = pid;
  shader->vertex_shader_id = vertex_shader;
  shader->fragment_shader_id = fragment_shader;

  return shader;
}

ListEntry* list_append(List* list, UiInstance* instance) {
  ListEntry* entry = malloc(sizeof(ListEntry));
  entry->prev = NULL;
  entry->next = NULL;
  entry->window = instance->window;
  entry->instance = instance;
  if(list->head == NULL) {
    list->head = entry;
    list->tail = entry;
  } else {
    ListEntry* prev_tail = list->tail;
    prev_tail->next = entry;
    entry->prev = prev_tail;
    list->tail = entry;
  }
  list->size++;

  return entry;
}
void list_remove(List* list, ListEntry* entry) {
  if(list->size == 1) {
    list->size = 0;
    list->head = NULL;
    list->tail = NULL;
  } else {
    ListEntry* prev = entry->prev;
    ListEntry* next = entry->next;
    if(prev != NULL && entry == list->tail){
      list->tail = prev;
      prev->next = NULL;
    } else if(next != NULL && entry == list->head){
      list->head = next;
      next->prev = NULL;
    } else if(prev != NULL && next != NULL) {
        next->prev = prev;
        prev->next = next;
    }
    list->size--;
  }
  free(entry);
}
ListEntry* list_find_window(List* list, GLFWwindow* window) {
  ListEntry* p = list->head;
  while(p != NULL){
    if(p->window == window)
      return p;
    p = p->next;
  }
  return NULL;
}
