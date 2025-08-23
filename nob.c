#define NOB_IMPLEMENTATION
#define NOB_STRIP_PREFIX
#define NOB_EXPERIMENTAL_DELETE_OLD
#include "nob.h"

#define BUILD_DIR "build"
#define SRC_DIR "examples"

bool build_game(Cmd* cmd, char *input, char *outupt) {
    nob_cc(cmd);
    nob_cc_inputs(cmd, input);
    nob_cc_output(cmd, outupt);
    nob_cc_flags(cmd);
    cmd_append(cmd, "-lraylib", "-lGL", "-lm", "-lpthread", "-ldl", "-lrt", "-lX11");
    return cmd_run_sync_and_reset(cmd);
}

int main(int argc, char** argv) {
    NOB_GO_REBUILD_URSELF(argc, argv);
    Cmd cmd = {0};

    if(!mkdir_if_not_exists(BUILD_DIR)) return 1;
    if(!build_game(&cmd, SRC_DIR"/snake.c", BUILD_DIR"/snake")) return 1;
    if(!build_game(&cmd, SRC_DIR"/with_raylib.c", BUILD_DIR"/with_raylib")) return 1;

    return 0;
}
