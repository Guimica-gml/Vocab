#ifndef COMP_H_
#define COMP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>

    typedef DWORD TIME;
    typedef HANDLE PID;
#else
    #include <unistd.h>
    #include <sys/wait.h>
    #include <sys/stat.h>

    typedef time_t TIME;
    typedef pid_t PID;
#endif

#define DA_DEFAULT_CAP 1024

// Used for *null terminated* strings
typedef const char *Cstr;

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} String;

typedef struct {
    Cstr *items;
    size_t count;
    size_t capacity;
} Da_Cstr;

typedef Da_Cstr Cmd;

#define da_append(da, item)                                             \
    do {                                                                \
        if ((da)->count >= (da)->capacity) {                            \
            size_t new_cap = ((da)->capacity == 0)                      \
                ? DA_DEFAULT_CAP                                        \
                : (da)->capacity * 2;                                   \
            (da)->items = realloc((da)->items, new_cap * sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Error: not enough RAM");     \
            (da)->capacity = new_cap;                                   \
        }                                                               \
        (da)->items[(da)->count++] = item;                              \
    } while(0)

#define da_append_many(da, items_ptr, items_count)                      \
    do {                                                                \
        if ((da)->capacity < (da)->count + (items_count)) {             \
            size_t new_cap = ((da)->capacity == 0)                      \
                ? DA_DEFAULT_CAP                                        \
                : (da)->capacity;                                       \
            while (new_cap < (da)->count + (items_count)) {             \
                new_cap *= 2;                                           \
            }                                                           \
            (da)->items = realloc((da)->items, new_cap * sizeof(*(da)->items)); \
            assert((da)->items != NULL && "Error: not enough RAM");     \
            (da)->capacity = new_cap;                                   \
        }                                                               \
        memcpy((da)->items + (da)->count, (items_ptr), (items_count) * sizeof(*(da)->items)); \
        (da)->count += (items_count);                                   \
    } while(0)

#define cmd_append(cmd, ...)                        \
    da_append_many(                                 \
        (cmd),                                      \
        ((Cstr[]){__VA_ARGS__}),                    \
        sizeof((Cstr[]){__VA_ARGS__})/sizeof(Cstr))

#define string_append_cstr(string, cstr)            \
    da_append_many(                                 \
        (string),                                   \
        (cstr),                                     \
        strlen(cstr))

#define string_append_null(string) \
    da_append((string), '\0')

#define rebuild_self(compiler_path, argc, argv)                 \
    rebuild_self_impl(compiler_path, (argc), (argv), __FILE__)  \

void rebuild_self_impl(Cstr compiler_path, int argc, const char **argv, Cstr source_filepath);
TIME get_last_time_modified(Cstr filepath);

int pid_wait(PID pid);

int cmd_exec(Cmd *cmd);
void cmd_exec_or_die(Cmd *cmd);

#endif // COMP_H_

#ifdef COMP_IMPLEMENTATION
#undef COMP_IMPLEMENTATION

TIME get_last_time_modified(Cstr filepath) {
#ifdef _WIN32
    HANDLE handle = CreateFile(
        filepath,
        GENERIC_READ,
        0, NULL,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL, NULL);

    FILETIME last_time_modified = {0};
    if (!GetFileTime(handle, NULL, NULL, &last_time_modified)) {
        char err[256] = {0};
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
        fprintf(
            stderr, "Error: could not stat '%s': %s\n",
            filepath, err);
        exit(1);
    }

    CloseHandle(handle);
    return last_time_modified.dwHighDateTime;
#else
    struct stat file_stat = {0};
    int err = stat(filepath, &file_stat);
    if (err != 0) {
        fprintf(
            stderr, "Error: could not stat '%s': %s\n",
            filepath, strerror(errno));
        exit(1);
    }
    return file_stat.st_mtime;
#endif
}

void rebuild_self_impl(Cstr compiler_path, int argc, const char **argv, Cstr src_filepath) {
    assert(argc > 0);
    Cstr exe_filepath = argv[0];

    TIME exe_last_modified = get_last_time_modified(exe_filepath);
    TIME src_last_modified = get_last_time_modified(src_filepath);

    if (src_last_modified > exe_last_modified) {
        Cmd cmd = {0};

        String old_exe_filepath = {0};
        string_append_cstr(&old_exe_filepath, exe_filepath);
        string_append_cstr(&old_exe_filepath, ".old");
        string_append_null(&old_exe_filepath);

        cmd.count = 0;
        cmd_append(&cmd, "mv", exe_filepath, old_exe_filepath.items);
        cmd_exec_or_die(&cmd);

        cmd.count = 0;
        cmd_append(&cmd, compiler_path, "-o", exe_filepath, src_filepath);

        int result = cmd_exec(&cmd);
        if (result != 0) {
            cmd.count = 0;
            cmd_append(&cmd, "mv", old_exe_filepath.items, exe_filepath);
            cmd_exec_or_die(&cmd);
            return;
        }

        cmd.count = 0;
        cmd_append(&cmd, exe_filepath);
        for (int i = 1; i < argc; ++i) {
            cmd_append(&cmd, argv[i]);
        }
        cmd_exec_or_die(&cmd);

        exit(0);
    }
}

int pid_wait(PID pid) {
#ifdef _WIN32
    DWORD ret = WaitForSingleObject(pid, INFINITE);
    if (ret == WAIT_FAILED) {
        char err[256] = {0};
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
        fprintf(stderr, "Error: could not wait on command: %s\n", err);
        exit(1);
    }

    DWORD exit_code;
    if (!GetExitCodeProcess(pid, &exit_code)) {
        char err[256] = {0};
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
        fprintf(stderr, "Error: could not get process exit code: %s\n", err);
        exit(1);
    }

    CloseHandle(pid);
    return exit_code;
#else
    while (true) {
        int status = {0};
        if (waitpid(pid, &status, 0) < 0) {
            fprintf(stderr, "Error: could not wait on command (pid %d): %s\n", pid, strerror(errno));
            exit(1);
        }

        if (WIFEXITED(status)) {
            int exit_status = WEXITSTATUS(status);
            return exit_status;
        }

        if (WIFSIGNALED(status)) {
            fprintf(stderr, "Error: command process was terminated by (pid %d): %s\n", pid, strerror(errno));
            exit(1);
        }
    }
#endif
}

int cmd_exec(Cmd *cmd) {
    printf("[CMD]: ");
    for (size_t i = 0; i < cmd->count; ++i) {
        printf("%s", cmd->items[i]);
        printf("%s", (i >= cmd->count - 1) ? "\n" : " ");
    }

#ifdef _WIN32
    String args = {0};
    for (size_t i = 0; i < cmd->count; ++i) {
        string_append_cstr(&args, cmd->items[i]);
    }
    string_append_null(&args);

    PROCESS_INFORMATION info = {0};
    if (!CreateProcess(NULL, args.items, NULL, NULL, FALSE, NORMAL_PRIORITY_CLASS, NULL, NULL, NULL, &info)) {
        char err[256] = {0};
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(),
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), err, 255, NULL);
        fprintf(stderr, "Error: could not exec child process: %s\n", err);
        exit(1);
    }

    return pid_wait(info.hProcess);
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        fprintf(stderr, "Error: could not fork child process: %s\n", strerror(errno));
        exit(1);
    }

    if (cpid == 0) {
        Da_Cstr args = {0};
        da_append_many(&args, cmd->items, cmd->count);
        da_append(&args, (char*) NULL);

        if (execvp(args.items[0], (char *const *) args.items) < 0) {
            fprintf(stderr, "Error: could not exec child process: %s\n", strerror(errno));
            exit(1);
        }
        exit(0);
    }

    return pid_wait(cpid);
#endif
}

void cmd_exec_or_die(Cmd *cmd) {
    int result = cmd_exec(cmd);
    if (result != 0) {
        fprintf(stderr, "Error: command failed so the program must die\n");
        exit(1);
    }
}

#endif // COMP_IMPLEMENTATION
