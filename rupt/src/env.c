#include "rupt.h"

Env *env_new(Env *parent) {
    Env *e = calloc(1,sizeof(Env));
    e->parent = parent;
    return e;
}
Value *env_get(Env *e, const char *name) {
    while (e) {
        for (int i=0;i<e->count;i++) if (strcmp(e->keys[i],name)==0) return e->vals[i];
        e = e->parent;
    }
    return NULL;
}
int env_has(Env *e, const char *name) {
    while (e) {
        for (int i=0;i<e->count;i++) if (strcmp(e->keys[i],name)==0) return 1;
        e = e->parent;
    }
    return 0;
}
void env_set(Env *e, const char *name, Value *v) {
    // if exists in current or parent? store in current scope (shadow)
    for (int i=0;i<e->count;i++) {
        if (strcmp(e->keys[i],name)==0) {
            val_release(e->vals[i]);
            e->vals[i] = val_retain(v);
            return;
        }
    }
    if (e->count == e->cap) {
        e->cap = e->cap? e->cap*2 : 8;
        e->keys = realloc(e->keys, e->cap*sizeof(char*));
        e->vals = realloc(e->vals, e->cap*sizeof(Value*));
    }
    e->keys[e->count] = strdup(name);
    e->vals[e->count] = val_retain(v);
    e->count++;
}
