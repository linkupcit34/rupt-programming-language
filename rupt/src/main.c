#include "rupt.h"
#include <stdio.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = 0;
    fclose(f);
    return buf;
}

static void run_source(const char *src, Env *env) {
    const char *p = src;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') p++;
    if (strncmp(p, "rupt", 4) != 0 || (p[4] != ' ' && p[4] != '\t' && p[4] != '\n' && p[4] != '\r' && p[4] != 0)) {
        fprintf(stderr, "??????: ???? ?????? ?????????? ?? ????? 'rupt' / Error: file must start with 'rupt'\n");
        return;
    }
    ASTNode *prog = parse_source(src);
    Frame fr; fr.signal = 0; fr.retval = NULL; fr.exc = NULL;
    exec_block(prog, env, &fr);
    if (fr.signal == 4) {
        fprintf(stderr, "??????: ... / Error: ...\n");
    }
    printf("by cit\n");
}

static void repl(Env *env) {
    char line[4096];
    printf("Rupt REPL (type 'exit' to quit)\n");
    for (;;) {
        printf(">>> ");
        if (!fgets(line, sizeof(line), stdin)) break;
        if (strncmp(line, "exit", 4) == 0) break;
        run_source(line, env);
    }
}

int main(int argc, char **argv) {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);
    Env *env = env_new(NULL);
    // builtins
    // print is handled in eval by name; but store a placeholder? Not needed.
    // Add math functions to global env as needed? For import math, file-based.
    if (argc == 1) {
        repl(env);
        return 0;
    }
    // parse args
    if (strcmp(argv[1], "run") == 0 && argc >= 3) {
        char *src = read_file(argv[2]);
        if (!src) { fprintf(stderr, "Не удалось открыть файл %s / Cannot open file %s\n", argv[2]); return 1; }
        run_source(src, env);
        free(src);
        return 0;
    }
    // rupt file.rupt
    char *src = read_file(argv[1]);
    if (src) {
        run_source(src, env);
        free(src);
        return 0;
    }
    fprintf(stderr, "Использование: rupt [run] <file.rupt> / Usage: rupt [run] <file.rupt>\n");
    return 1;
}

