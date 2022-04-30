#include <SDL2/SDL.h>
#include <gl/glew.h>
#include <SDL2/SDL_opengl.h>
#include <GL/GLU.h>


#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "imgui_impl_opengl3.h"


#include <cstdio>
#include <string.h>

char *slurp_file(const char *file_path) {
#define SLURP_FILE_PANIC \
    do { \
        fprintf(stderr, "Could not read file `%s`: %s\n", file_path, strerror(errno)); \
        exit(1); \
    } while (0)

    FILE *f = fopen(file_path, "r");
    if (f == NULL) SLURP_FILE_PANIC;
    if (fseek(f, 0, SEEK_END) < 0) SLURP_FILE_PANIC;

    long size = ftell(f);
    if (size < 0) SLURP_FILE_PANIC;

    char *buffer = static_cast<char*>(calloc(size + 1, sizeof(char)));
    if (buffer == NULL) SLURP_FILE_PANIC;

    if (fseek(f, 0, SEEK_SET) < 0) SLURP_FILE_PANIC;

    fread(buffer, 1, size, f);
    if (ferror(f) < 0) SLURP_FILE_PANIC;

    buffer[size] = '\0';

    if (fclose(f) < 0) SLURP_FILE_PANIC;

    return buffer;
#undef SLURP_FILE_PANIC
}


constexpr inline void _checkp (const char* file, const int line, const void* p) {
  if (!p) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s:%d: %s", file, line, SDL_GetError());
    exit(1);
  }
}
#define checkp(p) _checkp(__FILE__, __LINE__, static_cast<void*>((p)))

void _check (const char* file, const int line, const int e) {
  if (e < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s:%d: %s", file, line, SDL_GetError());
    exit(1);
  }
}
#define check(e) _check(__FILE__, __LINE__, (e))

GLuint compile_shader_from_source(const char* src, GLenum type) {
  GLuint shader {glCreateShader(type)};

  glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint shader_compiled = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compiled);
    if (shader_compiled != GL_TRUE) {
      //Shader log length
      int infoLogLength = 0;
      int maxLength = infoLogLength;
      
      //Get info string length
      glGetShaderiv( shader, GL_INFO_LOG_LENGTH, &maxLength );
      
      //Allocate string
      char* infoLog = new char[ maxLength ];
      
      //Get info log
      glGetShaderInfoLog(shader, maxLength, &infoLogLength, infoLog);
      if (infoLogLength > 0) {
        //Print Log
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, infoLog);
      }

      //Deallocate string
      delete[] infoLog;
      exit(1);
    }

  return shader;
}


#define DEFAULT_WIDTH 800
#define DEFAULT_HEIGHT 600
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

  // create the sdf shader

  GLuint program_id;
  GLuint vbo;
  GLuint ibo;
  GLint vert_pos_attrib;

  #define NUM_UNIFORMS 6
  GLint uniform_locs[NUM_UNIFORMS] {0};
  constexpr const char* uniform_names[NUM_UNIFORMS] = {
    "u_resolution",
    "u_max_steps",
    "u_max_dist",
    "u_surf_dist",
    "u_slider",
    "u_camera_pos",
  };
  
  #if NUM_UNIFORMS != 6
  #error "exhaustive handling of uniforms"
  #endif
  enum Uniform {
    U_RESOLUTION = 0,
    U_MAX_STEPS,
    U_MAX_DIST,
    U_SURF_DIST,
    U_SLIDER,
    U_CAMERA_POS,
  };
  {
    program_id = glCreateProgram();
    
    constexpr const char* vertex_src {
      "#version 330 core\n"
      "layout (location = 0) in vec2 pos;\n"
      "void main(void)\n{"
      "  gl_Position = vec4(pos, 0.0, 1.0);\n"
      "}"
    };
    GLuint vertex_shader {
      compile_shader_from_source(vertex_src, GL_VERTEX_SHADER)
    };
    
    const char* fragment_src { slurp_file("./src/fragment.glsl") };
    GLuint fragment_shader {
      compile_shader_from_source(fragment_src, GL_FRAGMENT_SHADER)
    };

    glAttachShader(program_id, vertex_shader);
    glAttachShader(program_id, fragment_shader);

    glLinkProgram(program_id);

    vert_pos_attrib = glGetAttribLocation(program_id, "pos");

    //Initialize clear color
    glClearColor( 0.f, 0.f, 0.f, 1.f );
    
    //VBO data
    GLfloat vertexData[] {
      -1.f, -1.f,
      1.f, -1.f,
      1.f,  1.f,
      -1.f,  1.f
    };
    
    //IBO data
    GLuint indexData[] { 0, 1, 2, 3 };
    
    //Create VBO
    glGenBuffers( 1, &vbo);
    glBindBuffer( GL_ARRAY_BUFFER, vbo);
    glBufferData( GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW );
    
    //Create IBO
    glGenBuffers( 1, &ibo);
    glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo);
    glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW );

    // get uniform locations
    for (int i = 0; i < NUM_UNIFORMS; ++i)
      uniform_locs[i] = glGetUniformLocation(program_id, uniform_names[i]);
  }
  
  // program state
  int ray_march_max_steps {100};
  float ray_march_max_dist {100.0};
  float ray_march_surf_dist {0.01f};
  float slider_values[4] {0.5f, 0.5f, 0.5f, 0.5f};
  float camera_pos[3] {0.0f, 1.0f, 0.0f};
  float camera_speed {0.5f};

  bool should_quit {false};
  bool fullscreen {false};
  SDL_Event e {0};
  while (!should_quit) {
    while (SDL_PollEvent(&e) != 0) {
      ImGui_ImplSDL2_ProcessEvent(&e);
      if (e.type == SDL_QUIT ||
          (e.type == SDL_WINDOWEVENT &&
           e.window.event == SDL_WINDOWEVENT_CLOSE &&
           e.window.windowID == SDL_GetWindowID(window)))
        should_quit = true;
      else if (e.type == SDL_KEYDOWN) {
        switch (e.key.keysym.sym) {
        case SDLK_ESCAPE: {
          should_quit = true;
        } break;
        case SDLK_F11: {
          fullscreen = !fullscreen;
        } break;
        case SDLK_w: {
          camera_pos[2] += camera_speed;
        } break;
        case SDLK_s: {
          camera_pos[2] -= camera_speed;
        } break;
        case SDLK_d: {
          camera_pos[0] += camera_speed;
        } break;
        case SDLK_a: {
          camera_pos[0] -= camera_speed;
        } break;
        default: {
          SDL_Log("%s", e.key.keysym.sym);
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

    // render ui
    // Start the Dear ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    { 
      ImGui::Begin("Settings");                          // Create a window called "Hello, world!" and append into it.
      
      ImGui::Checkbox("Fullscreen", &fullscreen);      // Edit bools storing our window open/close state

      ImGui::DragInt("max steps", &ray_march_max_steps, 0.0f, 1, 120);
      ImGui::DragFloat("max distance", &ray_march_max_dist, 0.0f, 1.0f, 100.0f);
      ImGui::DragFloat("surface distance", &ray_march_surf_dist, 0.0f, 0.0f, 1.0f);
      ImGui::DragFloat4("sliders", slider_values, 0.0f, -10.0f, 10.0f);
      
      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
      ImGui::End();
    }
    ImGui::Render();

    SDL_GL_GetDrawableSize(window, window_size, window_size+1);
    
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, window_size[0], window_size[1]);

    glUseProgram(program_id);

    // set uniforms
    #if NUM_UNIFORMS != 6
    #error "exhaustive handling of uniforms"
    #endif
    glUniform2f(uniform_locs[U_RESOLUTION],
                static_cast<float>(window_size[0]),
                static_cast<float>(window_size[1]));
    glUniform1i(uniform_locs[U_MAX_STEPS], ray_march_max_steps);
    glUniform1f(uniform_locs[U_MAX_DIST], ray_march_max_dist);
    glUniform1f(uniform_locs[U_SURF_DIST], ray_march_surf_dist);
    glUniform4f(uniform_locs[U_SLIDER], slider_values[0], slider_values[1], slider_values[2], slider_values[3]);
    glUniform3f(uniform_locs[U_CAMERA_POS], camera_pos[0], camera_pos[1], camera_pos[2]);
    
    glEnableVertexAttribArray(vert_pos_attrib);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(vert_pos_attrib, 2, GL_FLOAT,
                          GL_FALSE, 2 * sizeof(GLfloat),
                          NULL);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
    glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);

    glDisableVertexAttribArray(vert_pos_attrib);
    
    glUseProgram(0);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
  }
  
  return 0;
}
