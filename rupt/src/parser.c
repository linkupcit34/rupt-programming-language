#include "rupt.h"
#include <ctype.h>

// parser state
static Token *PT;
static int PP;
static int PTN;

static Token *cur(){ return &PT[PP]; }
static void adv(){ if (PP < PTN) PP++; }
static int match_type(TType t, const char *val) {
    if (cur()->type == t && (val==NULL || (cur()->text && strcmp(cur()->text,val)==0))) { adv(); return 1; }
    return 0;
}
static void expect_type(TType t, const char *val) {
    if (!match_type(t,val)) { fprintf(stderr,"Parse error at %d:%d expected %d %s got %d %s\n", cur()->line, cur()->col, t, val?val:"", cur()->type, cur()->text?cur()->text:""); exit(1); }
}

ASTNode *ast_new(NodeType t) {
    ASTNode *n = calloc(1,sizeof(ASTNode));
    n->type=t;
    return n;
}
static void addstmt(ASTNode *blk, ASTNode *s) {
    if (blk->body_count == blk->body_cap) { blk->body_cap = blk->body_cap?blk->body_cap*2:8; blk->body = realloc(blk->body, blk->body_cap*sizeof(ASTNode*)); }
    blk->body[blk->body_count++]=s;
}

// forward declarations
static ASTNode *parse_stmt();
static ASTNode *parse_expr();
static ASTNode *parse_block();

static ASTNode *parse_program() {
    ASTNode *p = ast_new(N_PROGRAM);
    // ensure body array
    p->body_cap=16; p->body=calloc(16,sizeof(ASTNode*));
    while (cur()->type == T_NEWLINE) adv();
    if (cur()->type == T_KEYWORD && cur()->text && strcmp(cur()->text,"rupt")==0) adv();
    while (cur()->type == T_NEWLINE) adv();
    while (cur()->type != T_EOF) {
        if (cur()->type == T_NEWLINE) { adv(); continue; }
        if (cur()->type == T_DEDENT) { adv(); continue; }
        ASTNode *s = parse_stmt();
        if (s) addstmt(p,s);
    }
    return p;
}

static ASTNode *parse_block() {
    ASTNode *b = ast_new(N_BLOCK);
    b->body_cap=16; b->body=calloc(16,sizeof(ASTNode*));
    if (cur()->type == T_INDENT) adv();
    while (cur()->type != T_DEDENT && cur()->type != T_EOF) {
        if (cur()->type == T_NEWLINE) { adv(); continue; }
        ASTNode *s = parse_stmt();
        if (s) addstmt(b,s);
    }
    if (cur()->type == T_DEDENT) adv();
    return b;
}

static ASTNode *parse_simple_stmt() {
    // typed decl: IDENT : type = expr
    if (cur()->type == T_IDENT && PP+1 < PTN && PT[PP+1].type == T_PUNCT && PT[PP+1].text && strcmp(PT[PP+1].text,":")==0) {
        ASTNode *n = ast_new(N_VARDECL);
        n->sval = strdup(cur()->text); adv();
        adv(); // :
        // type
        if (cur()->type == T_IDENT) { n->return_type?NULL:NULL; adv(); } // skip type
        if (match_type(T_OP, "=")) n->value = parse_expr();
        return n;
    }
    ASTNode *e = parse_expr();
    if (cur()->type == T_OP && cur()->text && (strcmp(cur()->text,"=")==0 || strcmp(cur()->text,"+=")==0 || strcmp(cur()->text,"-=")==0 || strcmp(cur()->text,"*=")==0 || strcmp(cur()->text,"/=")==0)) {
        char *op = strdup(cur()->text); adv();
        ASTNode *v = parse_expr();
        if (strcmp(op,"=")==0) {
            ASTNode *a = ast_new(N_ASSIGN);
            a->target = e; a->value = v; a->op = op;
            return a;
        } else {
            ASTNode *a = ast_new(N_AUGASSIGN);
            char *tmp = malloc(strlen(op)); memcpy(tmp, op, strlen(op)-1); tmp[strlen(op)-1]=0; a->op = tmp; // remove =
            return a;
        }
    }
    ASTNode *es = ast_new(N_EXPRSTMT);
    es->value = e; // store in value? We'll use expr field; add value as child? We'll set left? 
    // We'll store expression in 'test'? Use value pointer
    es->test = e;
    return es;
}

static ASTNode *parse_stmt() {
    if (cur()->type == T_KEYWORD) {
        if (strcmp(cur()->text,"if")==0) {
            adv();
            ASTNode *n = ast_new(N_IF);
            n->test = parse_expr();
            expect_type(T_PUNCT, ":");
            // skip newlines
            while (cur()->type == T_NEWLINE) adv();
            if (cur()->type == T_INDENT) n->block = parse_block(); else { n->block=ast_new(N_BLOCK); n->block->body_cap=4; n->block->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(n->block,s); }
            // elif/else
            n->elifs = NULL; n->elif_count=0; n->elif_cap=0;
            while (cur()->type == T_KEYWORD && (strcmp(cur()->text,"elif")==0 || strcmp(cur()->text,"else")==0)) {
                if (strcmp(cur()->text,"elif")==0) {
                    adv();
                    ASTNode *eif = ast_new(N_IF);
                    eif->test = parse_expr();
                    expect_type(T_PUNCT, ":");
                    while (cur()->type == T_NEWLINE) adv();
                    if (cur()->type == T_INDENT) eif->block = parse_block(); else { eif->block=ast_new(N_BLOCK); eif->block->body_cap=4; eif->block->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(eif->block,s); }
                    // append to elifs as ASTNode* (If node)
                    if (n->elif_count == n->elif_cap) { n->elif_cap = n->elif_cap?n->elif_cap*2:4; n->elifs = realloc(n->elifs, n->elif_cap*sizeof(ASTNode*)); }
                    n->elifs[n->elif_count++]=eif;
                } else {
                    adv();
                    expect_type(T_PUNCT, ":");
                    while (cur()->type == T_NEWLINE) adv();
                    if (cur()->type == T_INDENT) n->elseBody = parse_block(); else { n->elseBody=ast_new(N_BLOCK); n->elseBody->body_cap=4; n->elseBody->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(n->elseBody,s); }
                    break;
                }
            }
            return n;
        }
        if (strcmp(cur()->text,"while")==0) {
            adv();
            ASTNode *n = ast_new(N_WHILE);
            n->test = parse_expr();
            expect_type(T_PUNCT, ":");
            while (cur()->type == T_NEWLINE) adv();
            if (cur()->type == T_INDENT) n->block = parse_block(); else { n->block=ast_new(N_BLOCK); n->block->body_cap=4; n->block->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(n->block,s); }
            return n;
        }
        if (strcmp(cur()->text,"for")==0) {
            adv();
            ASTNode *n = ast_new(N_FOR);
            // target
            ASTNode *t = ast_new(N_IDENT); t->sval=strdup(cur()->text); adv();
            n->target = t;
            expect_type(T_KEYWORD, "in");
            n->value = parse_expr(); // iter stored in value
            expect_type(T_PUNCT, ":");
            while (cur()->type == T_NEWLINE) adv();
            if (cur()->type == T_INDENT) n->block = parse_block(); else { n->block=ast_new(N_BLOCK); n->block->body_cap=4; n->block->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(n->block,s); }
            return n;
        }
        if (strcmp(cur()->text,"fn")==0 || strcmp(cur()->text,"def")==0) {
            adv();
            ASTNode *n = ast_new(N_FN);
            n->sval = strdup(cur()->text); adv(); // name (IDENT)
            expect_type(T_PUNCT, "(");
            // params
            n->params=NULL; n->param_count=0; n->param_cap=0;
            n->defaults=NULL; n->default_count=0;
            if (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,")")==0)) {
                do {
                    if (cur()->type==T_OP && cur()->text && strcmp(cur()->text,"*")==0) { adv(); /* rest */ }
                    char *pname = strdup(cur()->text); adv();
                    if (n->param_count==n->param_cap){n->param_cap=n->param_cap?n->param_cap*2:4; n->params=realloc(n->params,n->param_cap*sizeof(char*));}
                    n->params[n->param_count++]=pname;
                    if (match_type(T_PUNCT, ":")) { /* type */ if (cur()->type==T_IDENT) adv(); }
                    if (match_type(T_OP, "=")) {
                        ASTNode *d = parse_expr();
                        if (n->default_count==0) { n->defaults=calloc(n->param_count,sizeof(ASTNode*)); }
                        // ensure size
                        // store default in defaults indexed by param_count-1
                        // realloc defaults to param_count
                        n->defaults = realloc(n->defaults, n->param_count*sizeof(ASTNode*));
                        n->defaults[n->param_count-1]=d;
                    }
                    if (!match_type(T_PUNCT, ",")) break;
                } while (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,")")==0));
            }
            expect_type(T_PUNCT, ")");
            if (match_type(T_OP, "->")) { /* return type */ if (cur()->type==T_IDENT) adv(); }
            expect_type(T_PUNCT, ":");
            while (cur()->type == T_NEWLINE) adv();
            if (cur()->type == T_INDENT) n->block = parse_block(); else { n->block=ast_new(N_BLOCK); n->block->body_cap=4; n->block->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(n->block,s); }
            return n;
        }
        if (strcmp(cur()->text,"class")==0) {
            adv();
            ASTNode *n = ast_new(N_CLASS);
            n->sval = strdup(cur()->text); adv();
            if (match_type(T_PUNCT, "(")) {
                while (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,")")==0)) {
                    // base expr
                    ASTNode *b = parse_expr();
                    // store in children?
                    ast_add_child(n,b);
                    if (!match_type(T_PUNCT, ",")) break;
                }
                expect_type(T_PUNCT, ")");
            }
            expect_type(T_PUNCT, ":");
            while (cur()->type == T_NEWLINE) adv();
            if (cur()->type == T_INDENT) n->block = parse_block(); else { n->block=ast_new(N_BLOCK); n->block->body_cap=4; n->block->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(n->block,s); }
            return n;
        }
        if (strcmp(cur()->text,"return")==0) {
            adv();
            ASTNode *n = ast_new(N_RETURN);
            if (!(cur()->type==T_NEWLINE || cur()->type==T_EOF || cur()->type==T_DEDENT)) n->value = parse_expr();
            return n;
        }
        if (strcmp(cur()->text,"import")==0) {
            adv();
            ASTNode *n = ast_new(N_IMPORT);
            // module name
            n->module = strdup(cur()->text); adv();
            while (match_type(T_PUNCT, ".")) {
                char buf[512]; snprintf(buf,512,"%s.%s",n->module,cur()->text); free(n->module); n->module=strdup(buf); adv();
            }
            // alias
            if (cur()->type==T_KEYWORD && strcmp(cur()->text,"as")==0) { adv(); /* alias name */ char *a=strdup(cur()->text); adv(); /* store alias? we'll store in names[0] */ n->names=calloc(1,sizeof(char*)); n->names[0]=a; n->name_count=1; }
            else { n->names=calloc(1,sizeof(char*)); n->names[0]=strdup(n->module); n->name_count=1; /* last part */ char *last=strrchr(n->module,'.'); if (last) n->names[0]=strdup(last+1); else n->names[0]=strdup(n->module); }
            return n;
        }
        if (strcmp(cur()->text,"from")==0) {
            adv();
            ASTNode *n = ast_new(N_FROMIMPORT);
            n->module = strdup(cur()->text); adv();
            while (match_type(T_PUNCT, ".")) { char buf[512]; snprintf(buf,512,"%s.%s",n->module,cur()->text); free(n->module); n->module=strdup(buf); adv(); }
            expect_type(T_KEYWORD, "import");
            // items
            n->names=NULL; n->name_count=0; n->aliases=NULL;
            do {
                char *iname = strdup(cur()->text); adv();
                n->name_count++;
                n->names = realloc(n->names, n->name_count*sizeof(char*));
                n->names[n->name_count-1]=iname;
                n->aliases = realloc(n->aliases, n->name_count*sizeof(char*));
                n->aliases[n->name_count-1]=NULL;
                if (match_type(T_KEYWORD, "as")) { n->aliases[n->name_count-1]=strdup(cur()->text); adv(); }
                if (!match_type(T_PUNCT, ",")) break;
            } while (cur()->type==T_IDENT || cur()->type==T_OP);
            return n;
        }
        if (strcmp(cur()->text,"try")==0) {
            adv();
            ASTNode *n = ast_new(N_TRY);
            expect_type(T_PUNCT, ":");
            while (cur()->type == T_NEWLINE) adv();
            if (cur()->type == T_INDENT) n->block = parse_block(); else { n->block=ast_new(N_BLOCK); n->block->body_cap=4; n->block->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(n->block,s); }
            // handlers
            // store first handler in value? We'll store in children? Simplify: only except Exception as e:
            while (cur()->type==T_KEYWORD && strcmp(cur()->text,"except")==0) {
                adv();
                ASTNode *h = ast_new(N_BLOCK); // handler body
                // exception type
                if (cur()->type==T_IDENT) adv();
                if (cur()->type==T_KEYWORD && strcmp(cur()->text,"as")==0) { adv(); if (cur()->type==T_IDENT) adv(); }
                expect_type(T_PUNCT, ":");
                while (cur()->type == T_NEWLINE) adv();
                if (cur()->type == T_INDENT) h = parse_block(); else { h=ast_new(N_BLOCK); h->body_cap=4; h->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(h,s); }
                // store handler body in elseBody? Use children
                ast_add_child(n,h);
                // mark as handler? We'll just store
            }
            if (cur()->type==T_KEYWORD && strcmp(cur()->text,"finally")==0) {
                adv(); expect_type(T_PUNCT,":");
                while (cur()->type == T_NEWLINE) adv();
                ASTNode *f;
                if (cur()->type == T_INDENT) f = parse_block(); else { f=ast_new(N_BLOCK); f->body_cap=4; f->body=calloc(4,sizeof(ASTNode*)); ASTNode*s=parse_stmt(); if(s)addstmt(f,s); }
                // store finally in elseBody
                n->elseBody = f;
            }
            return n;
        }
        if (strcmp(cur()->text,"raise")==0) {
            adv();
            ASTNode *n = ast_new(N_RAISE);
            if (!(cur()->type==T_NEWLINE||cur()->type==T_EOF||cur()->type==T_DEDENT)) n->value = parse_expr();
            return n;
        }
        if (strcmp(cur()->text,"assert")==0) {
            adv();
            ASTNode *n = ast_new(N_ASSERT);
            n->test = parse_expr();
            if (match_type(T_PUNCT, ",")) n->value = parse_expr();
            return n;
        }
        if (strcmp(cur()->text,"break")==0) { adv(); return ast_new(N_BREAK); }
        if (strcmp(cur()->text,"continue")==0) { adv(); return ast_new(N_CONTINUE); }
        if (strcmp(cur()->text,"pass")==0) { adv(); return ast_new(N_PASS); }
    }
    return parse_simple_stmt();
}

// Expression parsing
static ASTNode *parse_or();
static ASTNode *parse_and();
static ASTNode *parse_not();
static ASTNode *parse_cmp();
static ASTNode *parse_add();
static ASTNode *parse_mul();
static ASTNode *parse_pow();
static ASTNode *parse_unary();
static ASTNode *parse_postfix();
static ASTNode *parse_primary();

ASTNode *parse_expr(){ return parse_or(); }

static ASTNode *parse_or() {
    ASTNode *l = parse_and();
    while (cur()->type==T_KEYWORD && strcmp(cur()->text,"or")==0) { adv(); ASTNode *r=parse_and(); ASTNode *b=ast_new(N_BINARY); b->op=strdup("or"); b->left=l; b->right=r; l=b; }
    return l;
}
static ASTNode *parse_and() {
    ASTNode *l = parse_not();
    while (cur()->type==T_KEYWORD && strcmp(cur()->text,"and")==0) { adv(); ASTNode *r=parse_not(); ASTNode *b=ast_new(N_BINARY); b->op=strdup("and"); b->left=l; b->right=r; l=b; }
    return l;
}
static ASTNode *parse_not() {
    if (cur()->type==T_KEYWORD && strcmp(cur()->text,"not")==0) { adv(); ASTNode *n=ast_new(N_UNARY); n->op=strdup("not"); n->operand=parse_not(); return n; }
    return parse_cmp();
}
static ASTNode *parse_cmp() {
    ASTNode *l = parse_add();
    while ((cur()->type==T_OP && (strcmp(cur()->text,"==")==0||strcmp(cur()->text,"!=")==0||strcmp(cur()->text,"<")==0||strcmp(cur()->text,">")==0||strcmp(cur()->text,"<=")==0||strcmp(cur()->text,">=")==0)) ||
           (cur()->type==T_KEYWORD && (strcmp(cur()->text,"in")==0||strcmp(cur()->text,"is")==0))) {
        char *op = strdup(cur()->text); adv();
        if (strcmp(op,"is")==0 && cur()->type==T_KEYWORD && strcmp(cur()->text,"not")==0) { free(op); op=strdup("is not"); adv(); }
        ASTNode *r = parse_add();
        ASTNode *b=ast_new(N_BINARY); b->op=op; b->left=l; b->right=r; l=b;
    }
    return l;
}
static ASTNode *parse_add() {
    ASTNode *l = parse_mul();
    while (cur()->type==T_OP && (strcmp(cur()->text,"+")==0||strcmp(cur()->text,"-")==0)) {
        char *op=strdup(cur()->text); adv();
        ASTNode *r=parse_mul();
        ASTNode *b=ast_new(N_BINARY); b->op=op; b->left=l; b->right=r; l=b;
    }
    return l;
}
static ASTNode *parse_mul() {
    ASTNode *l = parse_pow();
    while (cur()->type==T_OP && (strcmp(cur()->text,"*")==0||strcmp(cur()->text,"/")==0||strcmp(cur()->text,"%")==0||strcmp(cur()->text,"//")==0)) {
        char *op=strdup(cur()->text); adv();
        ASTNode *r=parse_pow();
        ASTNode *b=ast_new(N_BINARY); b->op=op; b->left=l; b->right=r; l=b;
    }
    return l;
}
static ASTNode *parse_pow() {
    ASTNode *base = parse_unary();
    if (cur()->type==T_OP && cur()->text && strcmp(cur()->text,"**")==0) {
        adv();
        ASTNode *exp = parse_pow();
        ASTNode *b=ast_new(N_BINARY); b->op=strdup("**"); b->left=base; b->right=exp; return b;
    }
    return base;
}
static ASTNode *parse_unary() {
    if (cur()->type==T_OP && (strcmp(cur()->text,"-")==0||strcmp(cur()->text,"+")==0)) {
        char *op=strdup(cur()->text); adv();
        ASTNode *n=ast_new(N_UNARY); n->op=op; n->operand=parse_unary(); return n;
    }
    if (cur()->type==T_KEYWORD && strcmp(cur()->text,"lambda")==0) {
        adv();
        ASTNode *n=ast_new(N_LAMBDA);
        // params
        n->params=NULL; n->param_count=0; n->param_cap=0;
        if (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,":")==0)) {
            do {
                char *pname=strdup(cur()->text); adv();
                if (n->param_count==n->param_cap){n->param_cap=n->param_cap?n->param_cap*2:4; n->params=realloc(n->params,n->param_cap*sizeof(char*));}
                n->params[n->param_count++]=pname;
                if (!match_type(T_PUNCT,",")) break;
            } while (cur()->type==T_IDENT);
        }
        expect_type(T_PUNCT,":");
        n->block = ast_new(N_BLOCK); n->block->body_cap=1; n->block->body=calloc(1,sizeof(ASTNode*));
        ASTNode *e = parse_expr();
        n->block->body[n->block->body_count++]=e; // store expr as stmt? We'll handle lambda body as expression node in 'value'
        n->value = e;
        return n;
    }
    return parse_postfix();
}
static ASTNode *parse_postfix() {
    ASTNode *e = parse_primary();
    while (1) {
        if (cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,"(")==0) {
            adv();
            ASTNode *c = ast_new(N_CALL);
            c->target = e; // callee
            c->args=NULL; c->arg_count=0; c->arg_cap=0; c->arg_kind=NULL; c->arg_names=NULL;
            if (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,")")==0)) {
                do {
                    if (cur()->type==T_IDENT && PP+1<PTN && PT[PP+1].type==T_OP && PT[PP+1].text && strcmp(PT[PP+1].text,"=")==0) {
                        char *name=strdup(cur()->text); adv(); adv();
                        ASTNode *v=parse_expr();
                        // store as kw
                        c->arg_count++;
                        c->args=realloc(c->args,c->arg_count*sizeof(ASTNode*));
                        c->args[c->arg_count-1]=v;
                        if (!c->arg_kind){ c->arg_kind=calloc(c->arg_count,sizeof(int)); } else { c->arg_kind=realloc(c->arg_kind,c->arg_count*sizeof(int)); }
                        c->arg_kind[c->arg_count-1]=1;
                        if (!c->arg_names){ c->arg_names=calloc(c->arg_count,sizeof(char*)); } else { c->arg_names=realloc(c->arg_names,c->arg_count*sizeof(char*)); }
                        c->arg_names[c->arg_count-1]=name;
                    } else if (cur()->type==T_OP && cur()->text && strcmp(cur()->text,"*")==0) {
                        adv();
                        if (match_type(T_OP,"*")) {
                            ASTNode *v=parse_expr();
                            c->arg_count++;
                            c->args=realloc(c->args,c->arg_count*sizeof(ASTNode*));
                            c->args[c->arg_count-1]=v;
                            if (!c->arg_kind){ c->arg_kind=calloc(c->arg_count,sizeof(int)); } else { c->arg_kind=realloc(c->arg_kind,c->arg_count*sizeof(int)); }
                            c->arg_kind[c->arg_count-1]=3;
                        } else {
                            ASTNode *v=parse_expr();
                            c->arg_count++;
                            c->args=realloc(c->args,c->arg_count*sizeof(ASTNode*));
                            c->args[c->arg_count-1]=v;
                            if (!c->arg_kind){ c->arg_kind=calloc(c->arg_count,sizeof(int)); } else { c->arg_kind=realloc(c->arg_kind,c->arg_count*sizeof(int)); }
                            c->arg_kind[c->arg_count-1]=2;
                        }
                    } else {
                        ASTNode *v=parse_expr();
                        c->arg_count++;
                        c->args=realloc(c->args,c->arg_count*sizeof(ASTNode*));
                        c->args[c->arg_count-1]=v;
                        if (!c->arg_kind){ c->arg_kind=calloc(c->arg_count,sizeof(int)); } else { c->arg_kind=realloc(c->arg_kind,c->arg_count*sizeof(int)); }
                        c->arg_kind[c->arg_count-1]=0;
                        if (!c->arg_names){ c->arg_names=calloc(c->arg_count,sizeof(char*)); } else { c->arg_names=realloc(c->arg_names,c->arg_count*sizeof(char*)); }
                        c->arg_names[c->arg_count-1]=NULL;
                    }
                    if (!match_type(T_PUNCT,",")) break;
                } while (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,")")==0));
            }
            expect_type(T_PUNCT,")");
            e = c;
        } else if (cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,"[")==0) {
            adv();
            if (cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,":")==0) {
                // slice [:...]
                adv();
                ASTNode *sl = ast_new(N_SLICE);
                sl->target = e;
                sl->start = NULL;
                sl->stop = NULL; sl->step = NULL;
                if (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,"]")==0)) sl->stop = parse_expr();
                if (match_type(T_PUNCT,":")) { if (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,"]")==0)) sl->step = parse_expr(); }
                expect_type(T_PUNCT,"]");
                e = sl;
            } else {
                ASTNode *idx = parse_expr();
                if (match_type(T_PUNCT,":")) {
                    ASTNode *sl = ast_new(N_SLICE);
                    sl->target = e;
                    sl->start = idx;
                    sl->stop = NULL; sl->step = NULL;
                    if (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,"]")==0)) sl->stop = parse_expr();
                    if (match_type(T_PUNCT,":")) { if (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,"]")==0)) sl->step = parse_expr(); }
                    expect_type(T_PUNCT,"]");
                    e = sl;
                } else {
                    expect_type(T_PUNCT,"]");
                    ASTNode *ix = ast_new(N_INDEX);
                    ix->target = e; ix->value = idx; // store index in value
                    e = ix;
                }
            }
        } else if (cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,".")==0) {
            adv();
            ASTNode *a = ast_new(N_ATTR);
            a->target = e;
            a->sval = strdup(cur()->text); adv();
            e = a;
        } else break;
    }
    return e;
}

static ASTNode *parse_primary() {
    Token *t = cur();
    if (t->type == T_NUMBER) {
        ASTNode *n = ast_new(N_LITERAL);
        // int vs float
        if (strchr(t->text,'.') || strchr(t->text,'e') || strchr(t->text,'E')) { n->fval = t->num; n->ival = 0; n->bval = 1; /* mark float? use a flag */ }
        else { n->ival = (long long)t->num; n->fval = (double)n->ival; n->bval = 0; }
        // store whether float in op? We'll use bval as isFloat
        adv();
        return n;
    }
    if (t->type == T_STRING) {
        // interpolation?
        if (strchr(t->text,'{') && strchr(t->text,'}')) {
            ASTNode *n = ast_new(N_INTERPSTR);
            // store raw in sval
            n->sval = strdup(t->text);
            adv();
            return n;
        }
        ASTNode *n = ast_new(N_LITERAL);
        n->sval = strdup(t->text);
        // mark string: type? We'll use node type LITERAL and sval non-null; numbers have bval? We'll determine by sval presence and bval for float.
        // Use op="str"
        n->op = strdup("str");
        adv();
        return n;
    }
    if (t->type == T_KEYWORD && (strcmp(t->text,"true")==0||strcmp(t->text,"false")==0)) {
        ASTNode *n = ast_new(N_BOOL);
        n->bval = strcmp(t->text,"true")==0;
        adv();
        return n;
    }
    if (t->type == T_KEYWORD && strcmp(t->text,"none")==0) {
        adv(); return ast_new(N_NONE);
    }
    if (t->type == T_IDENT) {
        ASTNode *n = ast_new(N_IDENT);
        n->sval = strdup(t->text);
        adv();
        return n;
    }
    if (t->type == T_PUNCT && t->text && strcmp(t->text,"(")==0) {
        adv();
        if (match_type(T_PUNCT,")")) {
            ASTNode *n = ast_new(N_TUPLE); n->children=NULL; n->child_count=0; return n;
        }
        ASTNode *first = parse_expr();
        if (match_type(T_PUNCT,",")) {
            ASTNode *tu = ast_new(N_TUPLE);
            // items in children
            ast_add_child(tu, first);
            if (!(cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,")")==0)) {
                ASTNode *e = parse_expr(); ast_add_child(tu,e);
                while (match_type(T_PUNCT,",")) {
                    if (cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,")")==0) break;
                    ASTNode *e2 = parse_expr(); ast_add_child(tu,e2);
                }
            }
            expect_type(T_PUNCT,")");
            return tu;
        }
        expect_type(T_PUNCT,")");
        return first;
    }
    if (t->type == T_PUNCT && t->text && strcmp(t->text,"[")==0) {
        adv();
        if (match_type(T_PUNCT,"]")) {
            ASTNode *l = ast_new(N_LIST); return l;
        }
        ASTNode *first = parse_expr();
        if (cur()->type==T_KEYWORD && strcmp(cur()->text,"for")==0) {
            // comprehension
            adv();
            ASTNode *comp = ast_new(N_COMPREHENSION);
            comp->value = first; // expr
            // target
            ASTNode *tgt = ast_new(N_IDENT); tgt->sval=strdup(cur()->text); adv();
            comp->target = tgt;
            expect_type(T_KEYWORD,"in");
            comp->left = parse_expr(); // iter? store in left
            if (cur()->type==T_KEYWORD && strcmp(cur()->text,"if")==0) { adv(); comp->test = parse_expr(); }
            expect_type(T_PUNCT,"]");
            return comp;
        }
        ASTNode *l = ast_new(N_LIST);
        ast_add_child(l, first);
        while (match_type(T_PUNCT,",")) {
            if (cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,"]")==0) break;
            ASTNode *e = parse_expr();
            ast_add_child(l,e);
        }
        expect_type(T_PUNCT,"]");
        return l;
    }
    if (t->type == T_PUNCT && t->text && strcmp(t->text,"{")==0) {
        adv();
        if (match_type(T_PUNCT,"}")) {
            ASTNode *m = ast_new(N_MAP); return m;
        }
        ASTNode *first = parse_expr();
        if (match_type(T_PUNCT,":")) {
            ASTNode *val = parse_expr();
            if (cur()->type==T_KEYWORD && strcmp(cur()->text,"for")==0) {
                adv();
                ASTNode *comp = ast_new(N_COMPREHENSION);
                comp->value = first; // key? We'll store key in left, value in value
                comp->left = first; comp->value = val;
                ASTNode *tgt = ast_new(N_IDENT); tgt->sval=strdup(cur()->text); adv();
                comp->target = tgt;
                expect_type(T_KEYWORD,"in");
                comp->right = parse_expr(); // iter
                if (cur()->type==T_KEYWORD && strcmp(cur()->text,"if")==0) { adv(); comp->test = parse_expr(); }
                comp->op = strdup("map");
                expect_type(T_PUNCT,"}");
                return comp;
            }
            ASTNode *m = ast_new(N_MAP);
            // store entries in keys/values arrays
            // ensure capacity
            if (m->entry_cap==0){m->entry_cap=4; m->keys=calloc(4,sizeof(ASTNode*)); m->values=calloc(4,sizeof(ASTNode*));}
            m->keys[m->entry_count]=first; m->values[m->entry_count]=val; m->entry_count++;
            while (match_type(T_PUNCT,",")) {
                if (cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,"}")==0) break;
                ASTNode *k = parse_expr(); expect_type(T_PUNCT,":"); ASTNode *v = parse_expr();
                if (m->entry_count==m->entry_cap){m->entry_cap*=2; m->keys=realloc(m->keys,m->entry_cap*sizeof(ASTNode*)); m->values=realloc(m->values,m->entry_cap*sizeof(ASTNode*));}
                m->keys[m->entry_count]=k; m->values[m->entry_count]=v; m->entry_count++;
            }
            expect_type(T_PUNCT,"}");
            return m;
        }
        // set or set comprehension
        if (cur()->type==T_KEYWORD && strcmp(cur()->text,"for")==0) {
            adv();
            ASTNode *comp = ast_new(N_COMPREHENSION);
            comp->value = first;
            ASTNode *tgt = ast_new(N_IDENT); tgt->sval=strdup(cur()->text); adv();
            comp->target = tgt;
            expect_type(T_KEYWORD,"in");
            comp->left = parse_expr();
            if (cur()->type==T_KEYWORD && strcmp(cur()->text,"if")==0) { adv(); comp->test = parse_expr(); }
            comp->op = strdup("set");
            expect_type(T_PUNCT,"}");
            return comp;
        }
        ASTNode *s = ast_new(N_SET);
        ast_add_child(s, first);
        while (match_type(T_PUNCT,",")) {
            if (cur()->type==T_PUNCT && cur()->text && strcmp(cur()->text,"}")==0) break;
            ASTNode *e = parse_expr(); ast_add_child(s,e);
        }
        expect_type(T_PUNCT,"}");
        return s;
    }
    fprintf(stderr,"Unexpected token %d %s at %d:%d\n", t->type, t->text?t->text:"", t->line, t->col);
    exit(1);
}

// helper ast_add_child
void ast_add_child(ASTNode *p, ASTNode *c) {
    if (p->child_count == p->child_cap) { p->child_cap = p->child_cap?p->child_cap*2:8; p->children = realloc(p->children, p->child_cap*sizeof(ASTNode*)); }
    p->children[p->child_count++]=c;
}
void ast_add_stmt(ASTNode *p, ASTNode *c) { addstmt(p,c); }

ASTNode *parse_source(const char *src) {
    Lexer *L = lex_source(src);
    // convert Lexer to PT
    PT = L->toks;
    PTN = L->count;
    PP = 0;
    ASTNode *prog = parse_program();
    return prog;
}

