#ifndef _SHADER_H
#define _SHADER_H
#include "main.hpp"

struct shader_t {
  GLuint program;
  
  GLuint vert_shader; // this is cached because it doesn't change
  GLuint frag_shader; // in case the new shader can't compile
  
  GLuint vbo;
  GLuint ibo;
  
  GLint  vert_attrib;
  
  shader_t (void);
  void run (int [2], ray_march_params_t, float [4], const camera_t&);
  void recompile (void);
};

constexpr const char* fragment_path { ".\\src\\fragment.glsl" };

constexpr const char* vertex_src {
  "#version 330 core\n"
  "layout (location = 0) in vec2 pos;\n"
  "void main(void)\n{"
  "  gl_Position = vec4(pos, 0.0, 1.0);\n"
  "}"
};

#define NUM_UNIFORMS 7
static GLint uniform_locs[NUM_UNIFORMS] {0};
constexpr const char* uniform_names[NUM_UNIFORMS] = {
  "u_resolution",
  "u_max_steps",
  "u_max_dist",
  "u_surf_dist",
  "u_slider",
  "u_camera_pos",
  "u_mouse",
};
  
#if NUM_UNIFORMS != 7
#error "exhaustive handling of uniforms"
#endif
enum Uniform {
  U_RESOLUTION = 0,
  U_MAX_STEPS,
  U_MAX_DIST,
  U_SURF_DIST,
  U_SLIDER,
  U_CAMERA_POS,
  U_MOUSE
};
#endif // _SHADER_H
