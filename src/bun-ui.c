
#include "bun-ui.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

size_t g_init = 0;
size_t loaded_glad = 0;

int init() {
  if(g_init == 1)
    return 0;
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
    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  }
  ShaderVar vars[2] = {{2, sizeof(SimpleShaderEntry), GL_FLOAT, (void*)offsetof(SimpleShaderEntry, pos)},
      {2, sizeof(SimpleShaderEntry), GL_FLOAT, (void*)offsetof(SimpleShaderEntry, size)}};
  instance->shader = create_shader(IMAGE_SHADER_VERT, IMAGE_SHADER_FRAG, 16, vars, 2);
  image_buffer_resize(&(instance->render_buffer), buffer_w, buffer_h);
  allocate_texture(&(instance->render_buffer));
  glfwPollEvents();
  return instance;
}

uint8_t render_window(UiInstance* instance) {

  glfwMakeContextCurrent(instance->window);
  glfwGetFramebufferSize(instance->window, &instance->window_width, &instance->window_height);
  glViewport(0, 0, instance->window_width, instance->window_height);
  shader_set2f(instance->shader, "resolution", (float)instance->window_width, (float)instance->window_height);
  RgbaColor clear_color = instance->clear_color;
  glClearColor(clear_color.r / 255, clear_color.g / 255, clear_color.b /255, clear_color.a / 255);
  glClear(GL_COLOR_BUFFER_BIT);
  move_image_buffer_to_texture(&(instance->render_buffer));
  Vec2f window_size = {instance->window_width, instance->window_height};
  float t = window_size.x;
  if(window_size.y < window_size.x) 
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


Vec2f normalize(UiInstance* instance, Vec2f in) {
 Vec2f a = {.x = in.x - ((float)instance->window_width / 2), .y = in.y - ((float)instance->window_height/2)};
 return a;
}


uint8_t move_buffer_to_image(UiInstance* target, uint8_t* buffer, uint32_t w, uint32_t h) {
  const uint8_t pixel_size = target->render_buffer.type == RGBA ? 4 : 3;
  image_buffer_resize(&(target->render_buffer), w,h);
  memcpy((&target->render_buffer)->buffer, buffer, w*h*pixel_size);
  return 0;
}
void image_buffer_resize(Image* image, uint32_t w, uint32_t h) {
  const uint8_t pixel_size = image->type == RGBA ? 4 : 3;
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
  GLint color_t = buffer->type == RGBA ? GL_RGBA : GL_RGB;
  glTexImage2D(GL_TEXTURE_2D, 0, buffer->type == RGBA ? GL_RGBA8 : GL_RGB8, (GLsizei)buffer->w,
               (GLsizei)buffer->h, 0, color_t, GL_UNSIGNED_BYTE, NULL);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, buffer->w, buffer->h, color_t,
                  GL_UNSIGNED_BYTE, buffer->buffer);


}

uint8_t set_buffer_color_type(UiInstance* instance, uint8_t type) {
  Image* render_buffer = &instance->render_buffer;
  render_buffer->type = (enum ImageType)type;
  return 0;
}

uint8_t update_title(UiInstance* instance, const char* new_title) {
  glfwSetWindowTitle(instance->window, new_title);
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