#include "shader.hpp"

shader_t::shader_t() {
  program = glCreateProgram();
    
  {
    vert_shader = glCreateShader(GL_VERTEX_SHADER);
    GLuint shader = vert_shader;
    
    glShaderSource(shader, 1, &vertex_src, NULL);
    glCompileShader(shader);

    GLint shader_compiled { GL_FALSE };
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compiled);

    if (shader_compiled != GL_TRUE) {
      int log_len { 0 };
      int max_len { 0 };

      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);

      GLchar info_log[1024];

      glGetShaderInfoLog(shader, max_len, &log_len, info_log);
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", info_log);
      exit(1);
    }
  }
  
  {
    size_t size {0};
    char* fragment_src { slurp_file(fragment_path, &size) };
    frag_shader = glCreateShader(GL_FRAGMENT_SHADER);
    GLuint shader = frag_shader;
    
    glShaderSource(shader, 1, &fragment_src, &static_cast<int>(size));
    glCompileShader(shader);
    
    GLint shader_compiled { GL_FALSE };
    glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_compiled);
    if (shader_compiled != GL_TRUE) {
      int log_len { 0 };
      int max_len { 0 };
      
      glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &max_len);
      
      GLchar info_log[1024];
      
      glGetShaderInfoLog(shader, max_len, &log_len, info_log);
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", info_log);
      exit(1);
    }
    
    free(static_cast<void*>(fragment_src)); 
  }

  glAttachShader(program, vert_shader);
  glAttachShader(program, frag_shader);
  glLinkProgram(program);
  glDetachShader(program, vert_shader);
  glDetachShader(program, frag_shader);

  GLint linked;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (linked != GL_TRUE) {
    //GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(program, 1024, NULL, message);
    SDL_Log("%s", message);
    exit(1);
  }
  
  vert_attrib = glGetAttribLocation(program, "pos");

  // get uniform locations
  for (int i = 0; i < NUM_UNIFORMS; ++i)
    uniform_locs[i] = glGetUniformLocation(program, uniform_names[i]);

  //VBO data
  constexpr const GLfloat vertexData[] = {
    -1.f, -1.f,
    1.f, -1.f,
    1.f,  1.f,
    -1.f,  1.f
  };
  
  //IBO data
  constexpr const GLuint indexData[] { 0, 1, 2, 3 };
  
  //Create VBO
  glGenBuffers( 1, &vbo);
  glBindBuffer( GL_ARRAY_BUFFER, vbo);
  glBufferData( GL_ARRAY_BUFFER, 2 * 4 * sizeof(GLfloat), vertexData, GL_STATIC_DRAW );
  
  //Create IBO
  glGenBuffers( 1, &ibo);
  glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ibo);
  glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), indexData, GL_STATIC_DRAW );
}

void shader_t::recompile() {
  glDeleteProgram(program);
  program = glCreateProgram();
  
  {
    size_t size {0};
    char* fragment_src { slurp_file(fragment_path, &size) };
    GLuint new_shader { glCreateShader(GL_FRAGMENT_SHADER) };
    
    glShaderSource(new_shader, 1, &fragment_src, &static_cast<int>(size));
    glCompileShader(new_shader);

    free(static_cast<void*>(fragment_src)); 
    
    GLint shader_compiled { GL_FALSE };
    glGetShaderiv(new_shader, GL_COMPILE_STATUS, &shader_compiled);
    if (shader_compiled != GL_TRUE) {
      int log_len { 0 };
      int max_len { 0 };
      
      glGetShaderiv(new_shader, GL_INFO_LOG_LENGTH, &max_len);
      
      GLchar info_log[1024];
      
      glGetShaderInfoLog(new_shader, max_len, &log_len, info_log);
      SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s",info_log);
    } else {
      glDeleteShader(frag_shader);
      frag_shader = new_shader;
      SDL_Log("recompiled fragment shader");
    }
  }

  glAttachShader(program, vert_shader);
  glAttachShader(program, frag_shader);
  glLinkProgram(program);
  glDetachShader(program, vert_shader);
  glDetachShader(program, frag_shader);

  GLint linked;
  glGetProgramiv(program, GL_LINK_STATUS, &linked);
  if (linked != GL_TRUE) {
    //GLsizei log_length = 0;
    GLchar message[1024];
    glGetProgramInfoLog(program, 1024, NULL, message);
    SDL_Log("%s", message);
    exit(1);
  } else
    SDL_Log("program linked");
  
  vert_attrib = glGetAttribLocation(program, "pos");

  for (int i = 0; i < NUM_UNIFORMS; ++i)
    uniform_locs[i] = glGetUniformLocation(program, uniform_names[i]);
}

void shader_t::run(int window_size[2], ray_march_params_t rmp, float slider_values[4], const camera_t& camera) {
  glUseProgram(program);
#if NUM_UNIFORMS != 7
#error "exhaustive handling of uniforms"
#endif
  glUniform2f(uniform_locs[U_RESOLUTION], static_cast<float>(window_size[0]), static_cast<float>(window_size[1]));
  glUniform1i(uniform_locs[U_MAX_STEPS], rmp.max_steps);
  glUniform1f(uniform_locs[U_MAX_DIST], rmp.max_dist);
  glUniform1f(uniform_locs[U_SURF_DIST], rmp.surf_dist);
  glUniform4f(uniform_locs[U_SLIDER], slider_values[0], slider_values[1], slider_values[2], slider_values[3]);
  glUniform3f(uniform_locs[U_CAMERA_POS], camera.position.x, camera.position.y, camera.position.z);
  glUniform2f(uniform_locs[U_MOUSE], camera.yaw, camera.pitch);
  
  glEnableVertexAttribArray(vert_attrib);
  
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glVertexAttribPointer(vert_attrib, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(GLfloat), NULL);
  
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);
  glDrawElements(GL_TRIANGLE_FAN, 4, GL_UNSIGNED_INT, NULL);
  
  glDisableVertexAttribArray(vert_attrib);
  
  glUseProgram(0);
}
