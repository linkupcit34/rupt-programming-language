#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    
    const char *old = "strcmp(node->sval,\"jwt_decode\")==0";
    char *found = strstr(c, old);
    if (!found) {
        printf("old string not found\n");
        free(c);
        return 1;
    }
    
    printf("Found jwt_decode at offset %ld\n", found - c);
    
    const char *new = "strcmp(node->sval,\"jwt_decode\")==0 || strcmp(node->sval,\"mem_total\")==0 || strcmp(node->sval,\"mem_free\")==0 || strcmp(node->sval,\"mem_used\")==0 || strcmp(node->sval,\"mem_available\")==0 || strcmp(node->sval,\"mem_percent\")==0 || strcmp(node->sval,\"mem_lock\")==0 || strcmp(node->sval,\"mem_unlock\")==0 || strcmp(node->sval,\"mem_alloc\")==0 || strcmp(node->sval,\"mem_free_ptr\")==0";
    
    size_t old_len = strlen(old);
    size_t new_len = strlen(new);
    size_t c_len = strlen(c);
    size_t new_c_len = c_len - old_len + new_len;
    
    char *new_c = malloc(new_c_len + 1);
    memcpy(new_c, c, found - c);
    memcpy(new_c + (found - c), new, new_len);
    memcpy(new_c + (found - c) + new_len, found + old_len, c_len - (found - c) - old_len);
    new_c[new_c_len] = 0;
    
    FILE *out = fopen("eval.c", "w");
    fwrite(new_c, 1, new_c_len, out);
    fclose(out);
    
    free(c);
    free(new_c);
    printf("Added mem functions to identifier check\n");
    return 0;
}