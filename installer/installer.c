#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <sys/stat.h>
#include <sys/types.h>

static void ensure_dir(const char *p) {
    CreateDirectoryA(p, NULL);
}

static int file_exists(const char *p) {
    DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES;
}

int main(void) {
    char srcdir[MAX_PATH];
    GetModuleFileNameA(NULL, srcdir, MAX_PATH);
    char *last = strrchr(srcdir, '\\');
    if (last) *last = 0;

    char target[MAX_PATH];
    char *local = getenv("LOCALAPPDATA");
    if (!local) local = "C:\\Rupt";
    snprintf(target, MAX_PATH, "%s\\Rupt", local);

    printf("Установка Rupt / Installing Rupt\n");
    printf("Путь установки / Install path: %s\n", target);

    ensure_dir(target);
    char bin[MAX_PATH], std[MAX_PATH], reg[MAX_PATH], bricks[MAX_PATH];
    snprintf(bin, MAX_PATH, "%s\\bin", target);
    snprintf(std, MAX_PATH, "%s\\std", target);
    snprintf(reg, MAX_PATH, "%s\\registry", target);
    snprintf(bricks, MAX_PATH, "%s\\bricks", target);
    ensure_dir(bin); ensure_dir(std); ensure_dir(reg); ensure_dir(bricks);

    char cmd[2048];
    // copy binaries
    char ruptexe[MAX_PATH], brickexe[MAX_PATH];
    snprintf(ruptexe, MAX_PATH, "%s\\rupt.exe", srcdir);
    snprintf(brickexe, MAX_PATH, "%s\\brick.exe", srcdir);
    if (file_exists(ruptexe)) {
        snprintf(cmd, 2048, "copy /Y \"%s\" \"%s\\\" >nul", ruptexe, bin);
        system(cmd);
    }
    if (file_exists(brickexe)) {
        snprintf(cmd, 2048, "copy /Y \"%s\" \"%s\\\" >nul", brickexe, bin);
        system(cmd);
    }
    // copy std, registry, bricks
    char srcstd[MAX_PATH], srcreg[MAX_PATH], srcbricks[MAX_PATH];
    snprintf(srcstd, MAX_PATH, "%s\\std", srcdir);
    snprintf(srcreg, MAX_PATH, "%s\\registry", srcdir);
    snprintf(srcbricks, MAX_PATH, "%s\\bricks", srcdir);
    if (file_exists(srcstd)) {
        snprintf(cmd, 2048, "xcopy /e /i /y \"%s\" \"%s\" >nul", srcstd, std);
        system(cmd);
    }
    if (file_exists(srcreg)) {
        snprintf(cmd, 2048, "xcopy /e /i /y \"%s\" \"%s\" >nul", srcreg, reg);
        system(cmd);
    }
    if (file_exists(srcbricks)) {
        snprintf(cmd, 2048, "xcopy /e /i /y \"%s\" \"%s\" >nul", srcbricks, bricks);
        system(cmd);
    }

    // add to PATH
    char pathcmd[4096];
    snprintf(pathcmd, 4096, "setx PATH \"%%PATH%%;%s\" >nul", bin);
    system(pathcmd);

    printf("Готово / Done. Перезапустите терминал / Restart terminal.\n");
    return 0;
}
