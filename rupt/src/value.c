#include "rupt.h"

Value *val_new(ValType t) {
    Value *v = calloc(1,sizeof(Value));
    v->type=t; v->refcount=1;
    return v;
}
Value *val_null(void){ return val_new(VAL_NULL); }
Value *val_bool(int b){ Value*v=val_new(VAL_BOOL); v->as.boolean=b?1:0; return v; }
Value *val_int(long long x){ Value*v=val_new(VAL_INT); v->as.integer=x; return v; }
Value *val_float(double x){ Value*v=val_new(VAL_FLOAT); v->as.floating=x; return v; }
Value *val_str(const char *s){ Value*v=val_new(VAL_STR); v->as.string=strdup(s?s:""); return v; }
Value *val_list(void){ Value*v=val_new(VAL_LIST); v->as.list.items=NULL; v->as.list.count=0; v->as.list.cap=0; return v; }
Value *val_map(void){ Value*v=val_new(VAL_MAP); v->as.map.keys=NULL; v->as.map.vals=NULL; v->as.map.count=0; v->as.map.cap=0; return v; }

void val_release(Value *v) {
    if (!v) return;
    if (--v->refcount > 0) return;
    switch (v->type) {
        case VAL_STR: free(v->as.string); break;
        case VAL_LIST:
            for (int i=0;i<v->as.list.count;i++) val_release(v->as.list.items[i]);
            free(v->as.list.items); break;
        case VAL_MAP:
            for (int i=0;i<v->as.map.count;i++){ free(v->as.map.keys[i]); val_release(v->as.map.vals[i]); }
            free(v->as.map.keys); free(v->as.map.vals); break;
        case VAL_FUNC:
            // don't free function? 
            break;
        default: break;
    }
    free(v);
}
Value *val_retain(Value *v){ if (v) v->refcount++; return v; }

int val_truthy(Value *v) {
    if (!v || v->type==VAL_NULL) return 0;
    switch (v->type) {
        case VAL_BOOL: return v->as.boolean;
        case VAL_INT: return v->as.integer != 0;
        case VAL_FLOAT: return v->as.floating != 0.0;
        case VAL_STR: return v->as.string && v->as.string[0]!=0;
        case VAL_LIST: return v->as.list.count != 0;
        case VAL_MAP: return v->as.map.count != 0;
        default: return 1;
    }
}

int val_equals(Value *a, Value *b) {
    if (!a || !b) return a==b;
    if (a->type != b->type) {
        // numeric coercion
        if ((a->type==VAL_INT||a->type==VAL_FLOAT)&&(b->type==VAL_INT||b->type==VAL_FLOAT)) {
            double da = a->type==VAL_INT?a->as.integer:a->as.floating;
            double db = b->type==VAL_INT?b->as.integer:b->as.floating;
            return da==db;
        }
        return 0;
    }
    switch (a->type) {
        case VAL_NULL: return 1;
        case VAL_BOOL: return a->as.boolean==b->as.boolean;
        case VAL_INT: return a->as.integer==b->as.integer;
        case VAL_FLOAT: return a->as.floating==b->as.floating;
        case VAL_STR: return strcmp(a->as.string,b->as.string)==0;
        case VAL_LIST:
            if (a->as.list.count != b->as.list.count) return 0;
            for (int i=0;i<a->as.list.count;i++) if (!val_equals(a->as.list.items[i],b->as.list.items[i])) return 0;
            return 1;
        default: return a==b;
    }
}

char *val_to_str(Value *v) {
    static char buf[4096];
    if (!v) return "None";
    switch (v->type) {
        case VAL_NULL: return "None";
        case VAL_BOOL: return v->as.boolean ? "True" : "False";
        case VAL_INT: snprintf(buf,4096,"%lld",v->as.integer); return buf;
        case VAL_FLOAT: snprintf(buf,4096,"%g",v->as.floating); return buf;
        case VAL_STR: return v->as.string;
        case VAL_LIST: {
            char tmp[4096]; tmp[0]='['; tmp[1]=0; int pos=1;
            for (int i=0;i<v->as.list.count;i++) {
                char *s = val_to_str(v->as.list.items[i]);
                if (i) { tmp[pos++]=','; tmp[pos++]=' '; }
                int ls = strlen(s); if (pos+ls<4090){ memcpy(tmp+pos,s,ls); pos+=ls; }
            }
            tmp[pos++]=']'; tmp[pos]=0;
            // need persistent buffer; use static
            static char lbuf[4096];
            strcpy(lbuf,tmp);
            return lbuf;
        }
        case VAL_MAP: return "{...}";
        case VAL_FUNC: return "<function>";
        case VAL_CLASS: return "<class>";
        case VAL_INSTANCE: return "<instance>";
        case VAL_RANGE: return "<range>";
        default: return "?";
    }
}

void val_print(Value *v) {
    printf("%s\n", val_to_str(v));
}

// list helpers
void list_append(Value *l, Value *v) {
    if (l->as.list.count == l->as.list.cap) {
        l->as.list.cap = l->as.list.cap? l->as.list.cap*2 : 8;
        l->as.list.items = realloc(l->as.list.items, l->as.list.cap*sizeof(Value*));
    }
    l->as.list.items[l->as.list.count++] = val_retain(v);
}
// map helpers
void map_set(Value *m, const char *key, Value *v) {
    for (int i=0;i<m->as.map.count;i++) {
        if (strcmp(m->as.map.keys[i],key)==0) {
            val_release(m->as.map.vals[i]);
            m->as.map.vals[i]=val_retain(v);
            return;
        }
    }
    if (m->as.map.count == m->as.map.cap) {
        m->as.map.cap = m->as.map.cap? m->as.map.cap*2:8;
        m->as.map.keys = realloc(m->as.map.keys, m->as.map.cap*sizeof(char*));
        m->as.map.vals = realloc(m->as.map.vals, m->as.map.cap*sizeof(Value*));
    }
    m->as.map.keys[m->as.map.count]=strdup(key);
    m->as.map.vals[m->as.map.count]=val_retain(v);
    m->as.map.count++;
}
Value *map_get(Value *m, const char *key) {
    for (int i=0;i<m->as.map.count;i++) if (strcmp(m->as.map.keys[i],key)==0) return m->as.map.vals[i];
    return NULL;
}
int map_has(Value *m, const char *key) {
    for (int i=0;i<m->as.map.count;i++) if (strcmp(m->as.map.keys[i],key)==0) return 1;
    return 0;
}
