#define COMP_IMPLEMENTATION
#include "./comp.h"

#define EXE_FILEPATH "./build/vocab"

#define RAYLIB_SRC_PATH "./deps/raylib-5.0/src/"
#define RAYLIB_LIB_PATH "./build/raylib-linux/"

const char *OLOKO_MEU = "oloko meu!";

const char *raylib_units[] = {
    "rcore",
    "raudio",
    "rglfw",
    "rmodels",
    "rshapes",
    "rtext",
    "rtextures",
    "utils",
};
size_t raylib_units_count = sizeof(raylib_units)/sizeof(raylib_units[0]);

void build_raylib(void) {
    Cmd cmd = {0};

    cmd_append(&cmd, "mkdir", "-p", RAYLIB_LIB_PATH);
    cmd_exec_or_die(&cmd);

    String lib = {0};
    string_append_cstr(&lib, RAYLIB_LIB_PATH);
    string_append_cstr(&lib, "libraylib.a\0");

    Cmd cmd_rl = {0};
    cmd_append(&cmd_rl, "ar", "-crs", lib.items);

    for (size_t i = 0; i < raylib_units_count; ++i) {
        String unit = {0};
        string_append_cstr(&unit, RAYLIB_SRC_PATH);
        string_append_cstr(&unit, raylib_units[i]);
        string_append_cstr(&unit, ".c\0");

        String obj = {0};
        string_append_cstr(&obj, RAYLIB_LIB_PATH);
        string_append_cstr(&obj, raylib_units[i]);
        string_append_cstr(&obj, ".o\0");

        cmd_append(&cmd_rl, obj.items);

        cmd.count = 0;
        cmd_append(&cmd, "gcc", "-I"RAYLIB_SRC_PATH"external/glfw/include");
        cmd_append(&cmd, "-DPLATFORM_DESKTOP");
        cmd_append(&cmd, "-c", unit.items);
        cmd_append(&cmd, "-o", obj.items);
        cmd_exec_or_die(&cmd);
    }

    cmd_exec_or_die(&cmd_rl);
}

#define CFLAGS "-Wall", "-Wextra", "-pedantic", "-ggdb", "-std=c11"
#define CLIBS "-I"RAYLIB_SRC_PATH, "-L"RAYLIB_LIB_PATH, "-lraylib", "-lm"

int main(int argc, const char **argv) {
    rebuild_self("gcc", argc, argv);

    if (access(RAYLIB_LIB_PATH"libraylib.a", F_OK) != 0) {
        build_raylib();
    }

    Cmd cmd = {0};
    cmd_append(&cmd, "gcc", CFLAGS, "-o", EXE_FILEPATH, "./main.c", CLIBS);
    cmd_exec_or_die(&cmd);

    if (argc > 1 && strcmp(argv[1], "run") == 0) {
        cmd.count = 0;
        cmd_append(&cmd, EXE_FILEPATH);
        for (int i = 2; i < argc; ++i) {
            cmd_append(&cmd, argv[i]);
        }
        cmd_exec(&cmd);
    }

    return 0;
}
