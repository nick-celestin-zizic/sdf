#ifndef _MAIN_H_
#define _MAIN_H_

#include <SDL2/SDL_opengl.h>
#include <GL/GLU.h>
#include <glm/glm.hpp>

#include "util.hpp"

#include <glm/glm.hpp>
#include <glm/ext.hpp>
using namespace glm;

#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600

struct ray_march_params_t {
  int   max_steps { 500     };
  float max_dist  { 5000.0f };
  float surf_dist { 0.001f  };
};

#define CAMERA_SPEED 12.5f
struct camera_t {
  float pitch    { 0.0f                   };
  float yaw      { 0.0f                   };
  
  vec4  position { 0.0f, 1.0f, 0.0f, 1.0f };
  vec4  velocity { 0.0f, 0.0f, 0.0f, 1.0f };
  
  bool  active   { false                  };
};

#endif // _MAIN_H_
