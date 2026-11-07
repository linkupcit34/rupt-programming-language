#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

static int is_dir(const char *p) {
    struct stat st;
    if (stat(p, &st) != 0) return 0;
    return S_ISDIR(st.st_mode);
}

static void ensure_dir(const char *p) {
    if (!is_dir(p)) mkdir(p);
}

static void cmd_list(void) {
    system("dir /b bricks 2>nul");
}

static void cmd_install(const char *name) {
    char src[512], dst[512];
    snprintf(src, 512, "registry/%s", name);
    snprintf(dst, 512, "bricks/%s", name);
    if (!is_dir(src)) {
        printf("Brick не найден в реестре / Brick not found in registry: %s\n", name);
        return;
    }
    if (is_dir(dst)) {
        printf("Brick уже установлен / Brick already installed: %s\n", name);
        return;
    }
    ensure_dir("bricks");
    char cmd[1024];
    snprintf(cmd, 1024, "xcopy /e /i /q %s %s >nul", src, dst);
    system(cmd);
    printf("Установлен / Installed: %s\n", name);
}

static void cmd_remove(const char *name) {
    char path[512];
    snprintf(path, 512, "bricks/%s", name);
    if (!is_dir(path)) {
        printf("Brick не установлен / Brick not installed: %s\n", name);
        return;
    }
    char cmd[1024];
    snprintf(cmd, 1024, "rmdir /s /q %s", path);
    system(cmd);
    printf("Удалён / Removed: %s\n", name);
}

static void cmd_search(const char *name) {
    char cmd[512];
    snprintf(cmd, 512, "dir /b registry 2>nul | findstr %s", name);
    system(cmd);
}

static void cmd_info(const char *name) {
    char path[512];
    snprintf(path, 512, "bricks/%s/rupt.toml", name);
    FILE *f = fopen(path, "r");
    if (!f) {
        snprintf(path, 512, "registry/%s/rupt.toml", name);
        f = fopen(path, "r");
    }
    if (!f) { printf("Нет информации / No info: %s\n", name); return; }
    char buf[256];
    while (fgets(buf, sizeof(buf), f)) printf("%s", buf);
    fclose(f);
}

static void cmd_publish(void) {
    printf("Публикация доступна только через registry.rupt.dev (заглушка) / Publishing only via registry.rupt.dev (stub)\n");
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Использование / Usage: brick <install|remove|update|list|search|publish|info> ...\n");
        return 0;
    }
    if (strcmp(argv[1], "list") == 0) cmd_list();
    else if (strcmp(argv[1], "install") == 0 && argc >= 3) cmd_install(argv[2]);
    else if (strcmp(argv[1], "remove") == 0 && argc >= 3) cmd_remove(argv[2]);
    else if (strcmp(argv[1], "search") == 0 && argc >= 3) cmd_search(argv[2]);
    else if (strcmp(argv[1], "info") == 0 && argc >= 3) cmd_info(argv[2]);
    else if (strcmp(argv[1], "publish") == 0) cmd_publish();
    else if (strcmp(argv[1], "update") == 0 && argc >= 3) { cmd_remove(argv[2]); cmd_install(argv[2]); }
    else printf("Неизвестная команда / Unknown command\n");
    return 0;
}
