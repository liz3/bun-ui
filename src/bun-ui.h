#ifndef BUN_UI_H
#define BUN_UI_H

#include "glad.h"
#include <GLFW/glfw3.h>
#include "la.h"

#define IMAGE_SHADER_VERT "#version 330 core\n"\
"uniform vec2 resolution;\n"\
"layout(location = 0) in vec2 position;\n"\
"layout(location = 1) in vec2 size;\n"\
"vec2 camera_project(vec2 point) {\n"\
"  return 2 * (point) * (1 / resolution);\n"\
"}\n"\
"out vec2 uv;\n"\
"void main() {\n"\
"vec2 uvIn = vec2(float(gl_VertexID & 1),\n"\
"              float((gl_VertexID >> 1) & 1));\n"\
"    uv = uvIn;\n"\
"    vec2 r = camera_project(uvIn * size + position);\n"\
"    r.y *= -1;\n"\
"   gl_Position = vec4(r, 0.0f, 1.0f);\n"\
"}" 

#define IMAGE_SHADER_FRAG "#version 330 core\n"\
"uniform sampler2D img;\n"\
"in vec2 uv;\n"\
"out vec4 color;\n"\
"void main() {\n"\
"  color = texture(img, uv);\n"\
"} \n"

typedef struct {
    size_t len;
    uint8_t* buffer;
} String;

enum ImageType {
    RGBA,
    RGB,
    BGRA
};
typedef struct {
    uint32_t w,h;
    uint8_t* buffer;
    enum ImageType type;
    GLuint texture_id;
    size_t buffer_size;
    uint8_t texture_was_allocated;
} Image;

typedef struct {
    uint8_t r,g,b;
} RgbColor;

typedef struct {
    uint8_t r,g,b,a;
} RgbaColor;



typedef struct {
  uint32_t count;
  uint32_t size;
  uint16_t type;
  void* offset;
} ShaderVar;

typedef struct {
  Vec2f pos;
  Vec2f size;
} SimpleShaderEntry;

typedef struct {
    GLuint pid, vertex_shader_id, fragment_shader_id, vao, vbo;

} Shader;

typedef struct {
    GLFWwindow* window;
    int32_t window_width, window_height;
    Image render_buffer;
    RgbaColor clear_color;
    uint8_t should_center;
    uint8_t is_managed;
    Shader* shader;
    void* close_callback;
    void* key_callback;
    void* text_callback;
    void* framebuffer_size_callback;
    void* mouse_position_callback;
    void* mouse_button_callback;
    void* window_focus_callback;
    void* list_entry;
} UiInstance;

typedef struct ListEntry_t {
    GLFWwindow* window;
    UiInstance* instance;
    struct ListEntry_t* prev;
    struct ListEntry_t* next;
} ListEntry;

typedef struct List {
    size_t size;
    ListEntry* head;
    ListEntry* tail;
} List;

typedef uint8_t close_callback(UiInstance*);
typedef uint8_t key_cb_t(UiInstance*, int32_t key, int32_t scancode, int32_t action, int32_t mods);
typedef uint8_t text_cb_t(UiInstance*, uint32_t cp);
typedef uint8_t framebuffer_cb_t(UiInstance*, int32_t w, int32_t h);
typedef uint8_t mouse_position_cb_t(UiInstance*, double x, double y);
typedef uint8_t mouse_button_callback_t(UiInstance*, int32_t button, int32_t action, int32_t mods);
typedef uint8_t window_focus_callback_t(UiInstance*, int32_t focused);

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void window_focus_callback(GLFWwindow* window, int focused);

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

void character_callback(GLFWwindow* window, unsigned int codepoint);

uint8_t update_title(UiInstance* instance, const char* new_title);

ListEntry* list_append(List* list, UiInstance* instance);

void list_remove(List* list, ListEntry* entry);

ListEntry* list_find_window(List* list, GLFWwindow* window);

uint8_t get_buffer_pixel_size(Image* in);

GLint get_type_enum(Image* in, uint8_t type);

uint8_t string_match(const char* lhs, const char* rhs);

uint8_t set_clear_color(UiInstance* instance, uint8_t r, uint8_t g, uint8_t b);

uint8_t set_buffer_color_type(UiInstance* instance, const char* type);

uint8_t set_keyboard_callback(UiInstance* instance, void* callback);
uint8_t set_text_callback(UiInstance* instance, void* callback);
uint8_t set_framebuffer_callback(UiInstance* instance, void* callback);
uint8_t set_mouse_position_callback(UiInstance* instance, void* callback);
uint8_t set_mouse_button_callback(UiInstance* instance, void* callback);
uint8_t set_window_focus_callback(UiInstance* instance, void* callback);
uint8_t await_events(UiInstance* instance);
uint8_t await_events_timeout(UiInstance* instance, double max);
uint8_t set_is_managed(UiInstance* instance, uint8_t is_managed);

uint8_t move_buffer_to_image(UiInstance* target, uint8_t* buffer, uint32_t w, uint32_t h);

uint8_t dispose_instance(UiInstance* instance);

GLuint simple_compile_shader(GLuint type, const char* content);

void shader_use(Shader* shader);

Shader* create_shader(const char* vertex_content, const char* fragment_content, uint32_t size, ShaderVar* vars, size_t shader_var_len);

void shader_set2f(Shader* shader, const char* name, float x, float y);
void shader_set4f(Shader* shader, const char* name, float x, float y, float z, float w);
void shader_set1f(Shader* shader, const char* name, float v);

int init();

void allocate_texture(Image* image);

void image_buffer_resize(Image* image, uint32_t w, uint32_t h);

void move_image_buffer_to_texture(Image* buffer);

Vec2f normalize(UiInstance* instance, Vec2f in);

UiInstance* create_window(char* window_title, size_t buffer_w, size_t buffer_h, size_t window_width, size_t window_height, void* close_callback);

uint8_t render_window(UiInstance* instance);

#endif