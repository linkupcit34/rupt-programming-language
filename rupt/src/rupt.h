#ifndef RUPT_H
#define RUPT_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>

typedef enum { VAL_NULL, VAL_BOOL, VAL_INT, VAL_FLOAT, VAL_STR, VAL_LIST, VAL_MAP, VAL_FUNC, VAL_CLASS, VAL_INSTANCE, VAL_MODULE, VAL_RANGE } ValType;

typedef struct ASTNode ASTNode;

typedef struct Env Env;

typedef struct Function {
    char *name;
    char **params;
    int param_count;
    ASTNode *body;
    Env *closure;
} Function;

typedef struct Class {
    char *name;
    char **method_names;
    Function **methods;
    int method_count;
    int method_cap;
} Class;

typedef struct Instance {
    Class *cls;
    char **fields;
    struct Value **field_vals;
    int field_count;
    int field_cap;
} Instance;

typedef struct Value {
    ValType type;
    int refcount;
    union {
        int boolean;
        long long integer;
        double floating;
        char *string;
        struct { struct Value **items; int count; int cap; } list;
        struct { char **keys; struct Value **vals; int count; int cap; } map;
        Function *func;
        Class *cls;
        Instance *inst;
        struct { long long cur, end, step; int done; } range;
    } as;
} Value;

Value *val_null(void);
Value *val_bool(int);
Value *val_int(long long);
Value *val_float(double);
Value *val_str(const char *);
Value *val_list(void);
Value *val_map(void);
Value *val_new(ValType t);
void list_append(Value *l, Value *v);
int map_has(Value *m, const char *key);
Value *map_get(Value *m, const char *key);
void map_set(Value *m, const char *key, Value *v);
void val_release(Value *);
Value *val_retain(Value *);
void val_print(Value *);
int val_truthy(Value *);
int val_equals(Value *, Value *);
char *val_to_str(Value *);

typedef enum {
    N_PROGRAM, N_BLOCK, N_EXPRSTMT, N_VARDECL, N_ASSIGN, N_AUGASSIGN, N_RETURN,
    N_IF, N_FOR, N_WHILE, N_FN, N_CLASS, N_TRY, N_RAISE, N_IMPORT, N_FROMIMPORT,
    N_MATCH, N_CASE, N_BREAK, N_CONTINUE, N_PASS, N_ASSERT,
    N_BINARY, N_UNARY, N_CALL, N_INDEX, N_SLICE, N_ATTR, N_LIST, N_TUPLE, N_SET, N_MAP,
    N_COMPREHENSION, N_LAMBDA, N_INTERPSTR, N_IDENT, N_LITERAL, N_NONE, N_BOOL
} NodeType;

struct ASTNode {
    NodeType type;
    struct ASTNode **children; int child_count; int child_cap;
    char *sval;
    long long ival;
    double fval;
    int bval;
    struct ASTNode **body; int body_count; int body_cap;
    struct ASTNode *test;
    struct ASTNode *elseBody;
    struct ASTNode *block;
    struct ASTNode **elifs; int elif_count; int elif_cap;
    char **params; int param_count; int param_cap;
    struct ASTNode **defaults; int default_count;
    int is_async;
    struct ASTNode *return_type;
    struct ASTNode *target;
    struct ASTNode *value;
    char *op;
    struct ASTNode **args; int arg_count; int arg_cap;
    int *arg_kind;
    char **arg_names;
    struct ASTNode *left; struct ASTNode *right;
    struct ASTNode *operand;
    char **bases; int base_count;
    struct ASTNode **handlers;
    char **names; int name_count;
    char **aliases;
    char *module;
    struct ASTNode **keys; struct ASTNode **values; int entry_count; int entry_cap;
    struct ASTNode *start; struct ASTNode *stop; struct ASTNode *step;
};

struct Env {
    char **keys; Value **vals; int count; int cap;
    Env *parent;
};

Env *env_new(Env *parent);
Value *env_get(Env *, const char *);
void env_set(Env *, const char *, Value *);
int env_has(Env *, const char *);

typedef enum {
    T_EOF, T_NEWLINE, T_INDENT, T_DEDENT,
    T_NUMBER, T_STRING, T_IDENT, T_KEYWORD,
    T_OP, T_PUNCT, T_UNKNOWN
} TType;
typedef struct {
    TType type;
    char *text;
    double num;
    int line, col;
} Token;
ASTNode *ast_new(NodeType);
void ast_add_child(ASTNode *, ASTNode *);
void ast_add_stmt(ASTNode *, ASTNode *);

typedef struct {
    int signal;
    Value *retval;
    Value *exc;
} Frame;

int exec_block(ASTNode *, Env *, Frame *);
typedef struct { Token *toks; int count; int cap; } Lexer;
ASTNode *parse_source(const char *src);
int exec_block(ASTNode *, Env *, Frame *);
Lexer *lex_source(const char *src);
int exec_block(ASTNode *, Env *, Frame *);
#endif
