#include "util.hpp"

constexpr inline void _checkp(const char* file, const int line, const void* p) {
  if (!p) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s:%d: %s", file, line, SDL_GetError());
    exit(1);
  }
}


constexpr inline void _check (const char* file, const int line, const int e) {
  if (e < 0) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s:%d: %s", file, line, SDL_GetError());
    exit(1);
  }
}

inline uint64_t get_last_modified_time(const char* path) {
  struct stat s;
  return (stat(path, &s) == 0) ? s.st_mtime : -1;
}

char* slurp_file(const char* file_path, size_t* size) {
#define SLURP_FILE_PANIC                                                     \
  do {                                                                       \
    char buf[256];                                                           \
    strerror_s(buf, sizeof(buf), errno);                                     \
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not read file `%s`: %s \n", file_path, buf); \
    exit(1);                                                                 \
  } while (0)
  
  FILE *f;
  fopen_s(&f, file_path, "r");
  if (f == NULL) SLURP_FILE_PANIC;
  if (fseek(f, 0, SEEK_END) < 0) SLURP_FILE_PANIC;
  
  *size = ftell(f);
  if (size < 0) SLURP_FILE_PANIC;
  
  char *buffer = static_cast<char*>(calloc(*size + 1, sizeof(char)));
  if (buffer == NULL) SLURP_FILE_PANIC;
  
  if (fseek(f, 0, SEEK_SET) < 0) SLURP_FILE_PANIC;
  
  fread(buffer, 1, *size, f);
  if (ferror(f) < 0) SLURP_FILE_PANIC;
  
  buffer[*size] = '\0';
  
  if (fclose(f) < 0) SLURP_FILE_PANIC;
  
  return buffer;
#undef SLURP_FILE_PANIC
}

