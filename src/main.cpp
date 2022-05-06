#include <SDL2/SDL.h>
#include <gl/glew.h>
#include <SDL2/SDL_opengl.h>
#include <GL/GLU.h>

#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>
#include <string.h>
#include <stdint.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "main.hpp"
#include "util.cpp"
#include "shader.cpp"


int main (int argc, char** argv) {
  (void) argc;
  (void) argv;

  // init SDL to work with OpenGL
  check(SDL_Init(SDL_INIT_VIDEO));

  check(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
  check(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1));
  check(SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE ));
  
  SDL_Window* window;
  checkp(window = SDL_CreateWindow("SDF in SDL",
                                   SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                   DEFAULT_WIDTH, DEFAULT_HEIGHT,
                                   SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN));
  
  SDL_GLContext gl_context;
  checkp(gl_context = SDL_GL_CreateContext(window));

  int window_size[2] { DEFAULT_WIDTH, DEFAULT_HEIGHT };

  if (glewInit() != GLEW_OK) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize GLEW");
    return 1;
  }

  check(SDL_GL_SetSwapInterval(1)); // enable vsync

  // set up ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init("#version 330");
  
  // program state
  shader_t           shader    {};
  camera_t           camera    {};
  ray_march_params_t rm_params {};
  
  uint64_t shader_last_modified { get_last_modified_time(fragment_path) };
  float    mouse_sensitivity    { 0.001f };
  float    slider_values[4]     { 0.5f, 0.5f, 0.5f, 0.5f };

  bool should_quit { false };
  bool fullscreen  { false };
  uint64_t now     { SDL_GetPerformanceCounter() };
  uint64_t last    { 0 };
  float dt         { 0.0f };
  SDL_Event e      { 0 };
  while (!should_quit) {
    while (SDL_PollEvent(&e) != 0) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      if (e.type == SDL_QUIT ||
          (e.type == SDL_WINDOWEVENT &&
           e.window.event == SDL_WINDOWEVENT_CLOSE &&
           e.window.windowID == SDL_GetWindowID(window)))
        should_quit = true;
      else if (e.type == SDL_MOUSEBUTTONDOWN) {
        if (e.button.button == SDL_BUTTON_RIGHT) {
          camera.active = !camera.active;
          check(SDL_SetRelativeMouseMode((SDL_bool) camera.active));
        }
      }
      else if (e.type == SDL_MOUSEMOTION) {
        if (camera.active) {
          constexpr float tau = 6.28318530718f;
          camera.yaw   += fmod((static_cast<float>(e.motion.xrel)*mouse_sensitivity), tau);
          camera.pitch = glm::clamp(camera.pitch-static_cast<float>(e.motion.yrel)*mouse_sensitivity, -1.0f, 1.0f); 
        }
      }
      else if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE: {
          if (e.key.state == SDL_PRESSED)
            should_quit = true;
        } break;
        case SDLK_F11: {
          if (e.key.state == SDL_PRESSED)
            fullscreen = !fullscreen;
        } break;
        case SDLK_w: {
          if (e.key.state == SDL_PRESSED)
            camera.velocity.z = CAMERA_SPEED;
          else
            camera.velocity.z = glm::min(camera.velocity.z, 0.0f);
        } break;
        case SDLK_s: {
          if (e.key.state == SDL_PRESSED)
            camera.velocity.z = -CAMERA_SPEED;
          else
            camera.velocity.z = glm::max(camera.velocity.z, 0.0f);
        } break;
        case SDLK_d: {
          if (e.key.state == SDL_PRESSED)
            camera.velocity.x = CAMERA_SPEED;
          else
            camera.velocity.x = glm::min(camera.velocity.x, 0.0f);
        } break;
        case SDLK_a: {
          if (e.key.state == SDL_PRESSED)
            camera.velocity.x = -CAMERA_SPEED;
          else
            camera.velocity.x = glm::max(camera.velocity.x, 0.0f);
        } break;
        case SDLK_SPACE: {
          if (e.key.state == SDL_PRESSED)
            camera.velocity.y = CAMERA_SPEED;
          else
            camera.velocity.y = glm::min(camera.velocity.y, 0.0f);
        } break;
        case SDLK_LCTRL: {
          if (e.key.state == SDL_PRESSED)
            camera.velocity.y = -CAMERA_SPEED;
          else
            camera.velocity.y = glm::max(camera.velocity.y, 0.0f);
        } break;
        default: {
        }
        }
      }
    }

    if (fullscreen) {
      SDL_DisplayMode display_info {0};
      check(SDL_GetCurrentDisplayMode(0, &display_info));
      SDL_SetWindowSize(window, display_info.w, display_info.h);
      check(SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP));
      check(SDL_SetWindowDisplayMode(window, NULL));
    } else {
      check(SDL_SetWindowFullscreen(window, 0));
      SDL_SetWindowSize(window, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    }

    {
      // update dt
      last = now;
      now  = SDL_GetPerformanceCounter();
      dt   = static_cast<float>(now - last) / static_cast<float>(SDL_GetPerformanceFrequency());
      
      // check if shader needs reloading
      const uint64_t file_modified = get_last_modified_time(fragment_path);
      if (shader_last_modified != file_modified) {
        shader.recompile();
        shader_last_modified = file_modified;
      }
      
      // calculate camera position
      using namespace glm;
      const quat q_yaw = angleAxis(camera.yaw, vec3(0, 1, 0));
      camera.position += q_yaw * camera.velocity * dt;
    }

    // render ui
    // Start the Dear ImGui frame
    {
      ImGui_ImplOpenGL3_NewFrame();
      ImGui_ImplSDL2_NewFrame();
      ImGui::NewFrame();
      
      ImGui::Begin("Settings");
      
      ImGui::Checkbox("Fullscreen", &fullscreen);

      ImGui::SliderInt("max steps", &rm_params.max_steps, 1, 10000);
      ImGui::SliderFloat("max distance", &rm_params.max_dist, 1.0f, 10000.0f);
      ImGui::SliderFloat("surface distance", &rm_params.surf_dist, 0.01f, 1.0f);
      ImGui::SliderFloat("mouse sensitivity", &mouse_sensitivity, 0.0001f, .005f);
      ImGui::SliderFloat4("sliders", slider_values, -10.0f, 10.0f);
      
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
      ImGui::Render();
    }
    
    SDL_GL_GetDrawableSize(window, window_size, window_size+1);
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, window_size[0], window_size[1]);
    
    shader.run(window_size, rm_params, slider_values, camera);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }
  
  return 0;
}
