#include <stdio.h>
#include <string.h>

int main() {
    FILE *f = fopen("eval.c", "r");
    if (!f) { printf("Cannot open eval.c\n"); return 1; }
    
    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *c = malloc(len + 1);
    fread(c, 1, len, f);
    c[len] = 0;
    fclose(f);
    
    char *p = strstr(c, "jwt_decode");
    if (p) {
        // Print 500 chars starting 200 chars before jwt_decode
        int start = (int)(p - c);
        if (start > 200) start -= 200;
        int end = start + 400;
        if (end > len) end = len;
        for (int i = start; i < end; i++) {
            putchar(c[i]);
        }
        printf("\n---END---\n");
    }
    
    free(c);
    return 0;
}