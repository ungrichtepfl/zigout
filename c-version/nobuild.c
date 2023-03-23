#define NOBUILD_IMPLEMENTATION
#include "nobuild.h"
#include <string.h>

#define BIN_DIR "bin"

#define EXE BIN_DIR "/cout"
#define SRC "cout.c"

#define CPPFLAGS "-MMD", "-MP"
#define CFLAGS "-Wall", "-Wextra", "-Wpedantic", "-Werror"
// NOTE: MODIFY SDL include directory depending on your installation:
#define SDL2LIB "-I/usr/include/SDL2 -D_REENTRANT", "-lSDL2", "-lSDL2_ttf"
#define LDFLAGS "-lm", SDL2LIB

void build_game(const int release) {
  MKDIRS(BIN_DIR);
  if (release) {
#ifndef _WIN32
    CMD("cc", CPPFLAGS, CFLAGS, "-O3", SRC, "-o", EXE, LDFLAGS);
#else
    CMD("cl.exe", , CPPFLAGS, CFLAGS, "-O3", SRC, "-o", EXE, LDFLAGS);
#endif
  } else {
#ifndef _WIN32
    CMD("cc", CPPFLAGS, CFLAGS, SRC, "-o", EXE, LDFLAGS);
#else
    CMD("cl.exe", , CPPFLAGS, CFLAGS, SRC, "-o", EXE, LDFLAGS);
#endif
  }
}

void run_game(void) { CMD(EXE); }

int main(int argc, char **argv) {
  GO_REBUILD_URSELF(argc, argv);

  const char *const envvar = "RELEASE";
  const char *const release_env = getenv(envvar);

  int release = 0;
  if (release_env) {
    release = !strcmp(release_env, "1");
  }

  build_game(release);

  if (argc > 1) {
    if (!strcmp(argv[1], "run")) {
      run_game();
    } else {
      printf("Nothing to do for \"%s %s\". Use \"%s run\" to build AND run the "
             "game.\n",
             argv[0], argv[1], argv[0]);
    }
  }

  return 0;
}
