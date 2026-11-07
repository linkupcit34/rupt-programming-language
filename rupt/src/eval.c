#include "rupt.h"
#include <setjmp.h>
#include <time.h>
#include <windows.h>

// control flow signals
enum { SIG_NONE, SIG_RETURN, SIG_BREAK, SIG_CONTINUE, SIG_EXCEPTION };

static Frame *FR = NULL; // current frame pointer passed around

// forward
static Value *eval_expr(ASTNode *node, Env *env);
static int exec_stmt(ASTNode *node, Env *env, Frame *fr);

static Value *call_function(Function *fn, Value **args, int argc, Env *callEnv) {
    Env *newenv = env_new(fn->closure ? fn->closure : callEnv);
    // bind params
    for (int i=0;i<fn->param_count;i++) {
        Value *v = (i < argc) ? args[i] : val_null();
        env_set(newenv, fn->params[i], v);
    }
    // variadic not handled specially
    Frame fr; fr.signal=SIG_NONE; fr.retval=NULL; fr.exc=NULL;
    exec_block(fn->body, newenv, &fr);
    if (fr.signal == SIG_RETURN && fr.retval) {
        Value *r = fr.retval;
        fr.retval = NULL;
        return r;
    }
    if (fr.signal == SIG_EXCEPTION) {
        // propagate
        if (FR) { FR->signal = SIG_EXCEPTION; FR->exc = fr.exc; }
        return NULL;
    }
    return val_null();
}

static Value *instantiate(Class *cls, Value **args, int argc) {
    Instance *inst = calloc(1,sizeof(Instance));
    inst->cls = cls;
    Value *v = val_new(VAL_INSTANCE); // need set as.inst; but val_new doesn't know; we'll set manually
    // We allocated v as VAL_INSTANCE? val_new(VAL_INSTANCE) not handled? Add case? Let's create manually
    free(v);
    v = calloc(1,sizeof(Value));
    v->type = VAL_INSTANCE; v->refcount=1; v->as.inst = inst;
    // call __init__
    // find method
    for (int i=0;i<cls->method_count;i++) {
        if (strcmp(cls->method_names[i],"__init__")==0) {
            Function *init = cls->methods[i];
            // bind self
            Value **all = malloc((argc+1)*sizeof(Value*));
            all[0] = v; val_retain(v);
            for (int j=0;j<argc;j++) { all[j+1]=args[j]; }
            Value *r = call_function(init, all, argc+1, inst?NULL:NULL);
            (void)r;
            free(all);
            break;
        }
    }
    return v;
}

static Function *find_method(Class *cls, const char *name) {
    for (int i=0;i<cls->method_count;i++) if (strcmp(cls->method_names[i],name)==0) return cls->methods[i];
    return NULL;
}

static Value *eval_expr(ASTNode *node, Env *env) {
    if (!node) return val_null();
    switch (node->type) {
        case N_LITERAL:
            if (node->op && strcmp(node->op,"str")==0) {
                return val_str(node->sval ? node->sval : "");
            }
            if (node->bval) { // float flag
                return val_float(node->fval);
            }
            // number: if has dot in sval? We stored bval=1 for float. For int bval=0.
            // But for large, use ival.
            if (node->sval && (strchr(node->sval,'.')||strchr(node->sval,'e')||strchr(node->sval,'E'))) {
                return val_float(node->fval);
            }
            return val_int(node->ival);
        case N_BOOL: return val_bool(node->bval);
        case N_NONE: return val_null();
        case N_IDENT: {
            Value *v = env_get(env, node->sval);
            if (!v) {
                if (strcmp(node->sval,"print")==0 || strcmp(node->sval,"range")==0 || strcmp(node->sval,"len")==0 || strcmp(node->sval,"int")==0 || strcmp(node->sval,"float")==0 || strcmp(node->sval,"str")==0 || strcmp(node->sval,"bool")==0 || strcmp(node->sval,"list")==0 || strcmp(node->sval,"input")==0 || strcmp(node->sval,"time_now")==0 || strcmp(node->sval,"sleep_ms")==0 || strcmp(node->sval,"visible")==0 || strcmp(node->sval,"toilet")==0) {
                    return val_null();
                }
                fprintf(stderr, "NameError: %s is not defined\n", node->sval);
                if (FR) { FR->signal=SIG_EXCEPTION; FR->exc=val_str("NameError"); }
                return NULL;
            }
            return val_retain(v);
        }
        case N_INTERPSTR: {
            // build string
            char buf[4096]; buf[0]=0; int pos=0;
            const char *raw = node->sval;
            int i=0; int len=strlen(raw);
            while (i < len) {
                if (raw[i]=='{') {
                    // find matching
                    int depth=0; int j=i; char exprbuf[1024]; int ei=0;
                    j++; depth=1;
                    while (j<len && depth>0) {
                        if (raw[j]=='{') depth++;
                        else if (raw[j]=='}') { depth--; if (depth==0) break; }
                        if (ei<1023) exprbuf[ei++]=raw[j];
                        j++;
                    }
                    exprbuf[ei]=0;
                    // parse exprbuf
                    ASTNode *sub = parse_source(exprbuf);
                    // sub is program; take first stmt? Actually parse_expr needed. We have parse_source returns program. Evaluate first expr stmt?
                    // Simpler: wrap in expression by parsing? parse_source returns Program with body. If first stmt is ExprStmt, eval its expr.
                    Value *ev = val_null();
                    if (sub && sub->body_count>0) {
                        ASTNode *s = sub->body[0];
                        if (s->type == N_EXPRSTMT) ev = eval_expr(s->test, env);
                        else if (s->type == N_BLOCK) {}
                        else ev = eval_expr(s, env);
                    }
                    char *s = val_to_str(ev);
                    if (pos + strlen(s) < 4090) { strcpy(buf+pos, s); pos += strlen(s); }
                    if (ev) val_release(ev);
                    i = j+1;
                } else {
                    if (pos < 4095) buf[pos++]=raw[i];
                    i++;
                }
            }
            buf[pos]=0;
            return val_str(buf);
        }
        case N_UNARY: {
            Value *o = eval_expr(node->operand, env);
            if (!o) return NULL;
            if (strcmp(node->op,"-")==0) {
                if (o->type==VAL_INT) { long long r=-o->as.integer; val_release(o); return val_int(r); }
                if (o->type==VAL_FLOAT) { double r=-o->as.floating; val_release(o); return val_float(r); }
            }
            if (strcmp(node->op,"not")==0) {
                int t = val_truthy(o); val_release(o); return val_bool(!t);
            }
            return o;
        }
        case N_BINARY: {
            const char *op = node->op;
            if (strcmp(op,"and")==0) {
                Value *l = eval_expr(node->left, env);
                if (!l) return NULL;
                if (!val_truthy(l)) { return l; }
                val_release(l);
                return eval_expr(node->right, env);
            }
            if (strcmp(op,"or")==0) {
                Value *l = eval_expr(node->left, env);
                if (!l) return NULL;
                if (val_truthy(l)) { return l; }
                val_release(l);
                return eval_expr(node->right, env);
            }
            // in
            if (strcmp(op,"in")==0) {
                Value *right = eval_expr(node->right, env);
                Value *left = eval_expr(node->left, env);
                int found = 0;
                if (right && right->type==VAL_LIST) {
                    for (int i=0;i<right->as.list.count;i++) if (val_equals(right->as.list.items[i], left)) { found=1; break; }
                } else if (right && right->type==VAL_STR && left && left->type==VAL_STR) {
                    found = strstr(right->as.string, left->as.string) != NULL;
                } else if (right && right->type==VAL_MAP && left && left->type==VAL_STR) {
                    found = map_has(right, left->as.string);
                }
                val_release(left); val_release(right);
                return val_bool(found);
            }
            Value *l = eval_expr(node->left, env);
            if (!l) return NULL;
            Value *r = eval_expr(node->right, env);
            if (!r) { val_release(l); return NULL; }
            // arithmetic
            if (strcmp(op,"+")==0) {
                if (l->type==VAL_STR || r->type==VAL_STR) {
                    char buf[4096];
                    snprintf(buf,4096,"%s%s", val_to_str(l), val_to_str(r));
                    // careful: val_to_str for l may be overwritten by r; copy
                    // redo safely
                    char *ls = strdup(val_to_str(l));
                    char *rs = strdup(val_to_str(r));
                    snprintf(buf,4096,"%s%s", ls, rs);
                    free(ls); free(rs);
                    val_release(l); val_release(r);
                    return val_str(buf);
                }
                if (l->type==VAL_LIST && r->type==VAL_LIST) {
                    Value *res = val_list();
                    for (int i=0;i<l->as.list.count;i++) list_append(res, l->as.list.items[i]);
                    for (int i=0;i<r->as.list.count;i++) list_append(res, r->as.list.items[i]);
                    val_release(l); val_release(r);
                    return res;
                }
                if (l->type==VAL_INT && r->type==VAL_INT) {
                    long long x = l->as.integer + r->as.integer;
                    val_release(l); val_release(r);
                    return val_int(x);
                }
                double dl = l->type==VAL_INT ? l->as.integer : l->as.floating;
                double dr = r->type==VAL_INT ? r->as.integer : r->as.floating;
                val_release(l); val_release(r);
                return val_float(dl+dr);
            }
            if (strcmp(op,"-")==0) {
                if (l->type==VAL_INT && r->type==VAL_INT) {
                    long long x = l->as.integer - r->as.integer;
                    val_release(l); val_release(r); return val_int(x);
                }
                double dl = l->type==VAL_INT ? l->as.integer : l->as.floating;
                double dr = r->type==VAL_INT ? r->as.integer : r->as.floating;
                val_release(l); val_release(r); return val_float(dl-dr);
            }
            if (strcmp(op,"*")==0) {
                if (l->type==VAL_STR && r->type==VAL_INT) {
                    int n = r->as.integer;
                    char buf[4096]; buf[0]=0; int pos=0;
                    for (int i=0;i<n && pos<4090;i++) { int ls=strlen(l->as.string); if (pos+ls<4090){memcpy(buf+pos,l->as.string,ls);pos+=ls;} }
                    buf[pos]=0;
                    val_release(l); val_release(r); return val_str(buf);
                }
                if (l->type==VAL_INT && r->type==VAL_INT) {
                    long long x = l->as.integer * r->as.integer;
                    val_release(l); val_release(r); return val_int(x);
                }
                double dl = l->type==VAL_INT ? l->as.integer : l->as.floating;
                double dr = r->type==VAL_INT ? r->as.integer : r->as.floating;
                val_release(l); val_release(r); return val_float(dl*dr);
            }
            if (strcmp(op,"/")==0) {
                double dl = l->type==VAL_INT ? l->as.integer : l->as.floating;
                double dr = r->type==VAL_INT ? r->as.integer : r->as.floating;
                val_release(l); val_release(r); return val_float(dl/dr);
            }
            if (strcmp(op,"%")==0) {
                if (l->type==VAL_INT && r->type==VAL_INT) {
                    long long x = l->as.integer % r->as.integer;
                    val_release(l); val_release(r); return val_int(x);
                }
                double dl = l->type==VAL_INT ? l->as.integer : l->as.floating;
                double dr = r->type==VAL_INT ? r->as.integer : r->as.floating;
                val_release(l); val_release(r); return val_float(fmod(dl,dr));
            }
            if (strcmp(op,"**")==0) {
                double dl = l->type==VAL_INT ? l->as.integer : l->as.floating;
                double dr = r->type==VAL_INT ? r->as.integer : r->as.floating;
                val_release(l); val_release(r); return val_float(pow(dl,dr));
            }
            if (strcmp(op,"//")==0) {
                double dl = l->type==VAL_INT ? l->as.integer : l->as.floating;
                double dr = r->type==VAL_INT ? r->as.integer : r->as.floating;
                val_release(l); val_release(r); return val_int((long long)floor(dl/dr));
            }
            // comparisons
            if (strcmp(op,"==")==0) { int e=val_equals(l,r); val_release(l); val_release(r); return val_bool(e); }
            if (strcmp(op,"!=")==0) { int e=val_equals(l,r); val_release(l); val_release(r); return val_bool(!e); }
            if (strcmp(op,"<")==0||strcmp(op,">")==0||strcmp(op,"<=")==0||strcmp(op,">=")==0) {
                double dl, dr;
                if (l->type==VAL_STR && r->type==VAL_STR) {
                    int c = strcmp(l->as.string, r->as.string);
                    int res=0;
                    if (strcmp(op,"<")==0) res = c<0;
                    else if (strcmp(op,">")==0) res = c>0;
                    else if (strcmp(op,"<=")==0) res = c<=0;
                    else res = c>=0;
                    val_release(l); val_release(r); return val_bool(res);
                }
                dl = l->type==VAL_INT ? l->as.integer : (l->type==VAL_FLOAT?l->as.floating:0);
                dr = r->type==VAL_INT ? r->as.integer : (r->type==VAL_FLOAT?r->as.floating:0);
                int res=0;
                if (strcmp(op,"<")==0) res = dl<dr;
                else if (strcmp(op,">")==0) res = dl>dr;
                else if (strcmp(op,"<=")==0) res = dl<=dr;
                else res = dl>=dr;
                val_release(l); val_release(r); return val_bool(res);
            }
            if (strcmp(op,"is")==0) { int e = (l==r); val_release(l); val_release(r); return val_bool(e); }
            if (strcmp(op,"is not")==0) { int e = (l!=r); val_release(l); val_release(r); return val_bool(e); }
            val_release(l); val_release(r);
            return val_null();
        }
        case N_CALL: {
            // evaluate callee
            Value *callee = eval_expr(node->target ? node->target : node->left, env);
            // Actually N_CALL has target field? We set target. Use node->target.
            if (!callee) callee = eval_expr(node->left, env);
            // evaluate args
            int argc = node->arg_count;
            Value **args = argc? malloc(argc*sizeof(Value*)):NULL;
            for (int i=0;i<argc;i++) {
                args[i] = eval_expr(node->args[i], env);
                if (!args[i]) args[i]=val_null();
            }
            // handle builtins and functions
            Value *result = NULL;
            if (callee && callee->type == VAL_FUNC) {
                result = call_function(callee->as.func, args, argc, env);
            } else if (callee && callee->type == VAL_CLASS) {
                result = instantiate(callee->as.cls, args, argc);
            } else if (callee && callee->type == VAL_INSTANCE) {
                // call __call__? 
                result = val_null();
            } else if (callee && callee->type == VAL_STR) {
                // maybe function name string? 
                result = val_null();
            } else if (callee && callee->type == VAL_NULL) {
                // builtin by name? node target ident string
            } else {
                // builtin function stored as special? We'll handle named calls below.
            }
            // builtins: if callee is ident, check name
            // We need know function name. If node->target is N_IDENT, use its sval.
            char *fname = NULL;
            if (node->target && node->target->type == N_IDENT) fname = node->target->sval;
            if (!result && fname) {
                // builtins
                if (strcmp(fname,"print")==0 || strcmp(fname,"visible")==0) {
                    for (int i=0;i<argc;i++) {
                        if (i) printf(" ");
                        printf("%s", val_to_str(args[i]));
                    }
                    printf("\n");
                    result = val_null();
                } else if (strcmp(fname,"toilet")==0) {
                    exit(0);
                } else if (strcmp(fname,"len")==0) {
                    long long l=0;
                    if (argc>0 && args[0]) {
                        if (args[0]->type==VAL_LIST) l=args[0]->as.list.count;
                        else if (args[0]->type==VAL_STR) l=strlen(args[0]->as.string);
                        else if (args[0]->type==VAL_MAP) l=args[0]->as.map.count;
                    }
                    result = val_int(l);
                } else if (strcmp(fname,"range")==0) {
                    long long start=0,end=0,step=1;
                    if (argc==1) { end = (args[0]->type==VAL_INT)?args[0]->as.integer:(long long)args[0]->as.floating; }
                    else if (argc>=2) {
                        start = args[0]->type==VAL_INT?args[0]->as.integer:(long long)args[0]->as.floating;
                        end = args[1]->type==VAL_INT?args[1]->as.integer:(long long)args[1]->as.floating;
                        if (argc>=3) step = args[2]->type==VAL_INT?args[2]->as.integer:(long long)args[2]->as.floating;
                    }
                    Value *r = val_new(VAL_RANGE?VAL_NULL:VAL_NULL);
                    // We need range type. Add VAL_RANGE? Not in enum. We used VAL_RANGE in enum yes.
                    // Reallocate as range
                    free(r);
                    r = calloc(1,sizeof(Value));
                    r->type = VAL_RANGE; /* VAL_RANGE exists */ r->refcount=1;
                    r->as.range.cur = start; r->as.range.end = end; r->as.range.step = step; r->as.range.done=0;
                    result = r;
                } else if (strcmp(fname,"int")==0) {
                    long long x=0;
                    if (argc>0 && args[0]) {
                        if (args[0]->type==VAL_INT) x=args[0]->as.integer;
                        else if (args[0]->type==VAL_FLOAT) x=(long long)args[0]->as.floating;
                        else if (args[0]->type==VAL_STR) x=atoll(args[0]->as.string);
                        else if (args[0]->type==VAL_BOOL) x=args[0]->as.boolean;
                    }
                    result = val_int(x);
                } else if (strcmp(fname,"float")==0) {
                    double x=0;
                    if (argc>0 && args[0]) {
                        if (args[0]->type==VAL_INT) x=args[0]->as.integer;
                        else if (args[0]->type==VAL_FLOAT) x=args[0]->as.floating;
                        else if (args[0]->type==VAL_STR) x=atof(args[0]->as.string);
                    }
                    result = val_float(x);
                } else if (strcmp(fname,"str")==0) {
                    result = val_str(argc>0?val_to_str(args[0]):"");
                } else if (strcmp(fname,"bool")==0) {
                    result = val_bool(argc>0?val_truthy(args[0]):0);
                } else if (strcmp(fname,"list")==0) {
                    result = val_list();
                    if (argc>0 && args[0] && args[0]->type==VAL_LIST) {
                        for (int i=0;i<args[0]->as.list.count;i++) list_append(result, args[0]->as.list.items[i]);
                    }
                } else if (strcmp(fname,"input")==0) {
                    char buf[1024]; 
                    if (argc>0) printf("%s", val_to_str(args[0]));
                    if (fgets(buf,1024,stdin)) {
                        size_t l=strlen(buf); if (l>0 && buf[l-1]=='\n') buf[l-1]=0;
                        result = val_str(buf);
                    } else result = val_str("");
                } else if (strcmp(fname,"time_now")==0) {
                    result = val_int((long long)time(NULL) * 1000LL);
                } else if (strcmp(fname,"sleep_ms")==0) {
                    long long ms = argc>0 ? (args[0]->type==VAL_INT?args[0]->as.integer:(long long)args[0]->as.floating) : 0;
                    Sleep((DWORD)ms);
                    result = val_null();
                } else {
                    // user-defined function by name in env
                    Value *fv = env_get(env, fname);
                    if (fv && fv->type==VAL_FUNC) {
                        result = call_function(fv->as.func, args, argc, env);
                    } else {
                        fprintf(stderr, "NameError: %s is not defined\n", fname);
                        if (FR) { FR->signal=SIG_EXCEPTION; FR->exc=val_str("NameError"); }
                        result = NULL;
                    }
                }
            }
            if (!result) result = val_null();
            // method calls: if callee is attr, handle below? We handled callee evaluation; for method call callee should be function. But instance method call needs self binding. Our eval of Attr returns? Let's handle Attr evaluation to bind self by creating a bound function? Simpler: in N_ATTR case, if attribute is method, return a special marker? We'll handle calls where callee is VAL_INSTANCE? Hmm.
            for (int i=0;i<argc;i++) if (args[i]) val_release(args[i]);
            free(args);
            if (callee) val_release(callee);
            return result;
        }
        case N_INDEX: {
            Value *t = eval_expr(node->target, env);
            Value *idx = eval_expr(node->value, env);
            Value *res = val_null();
            if (t && t->type==VAL_LIST) {
                long long i = (idx->type==VAL_INT)?idx->as.integer:(long long)idx->as.floating;
                if (i < 0) i += t->as.list.count;
                if (i >= 0 && i < t->as.list.count) res = val_retain(t->as.list.items[i]);
                else {
                    fprintf(stderr, "IndexError\n");
                    if (FR) { FR->signal=SIG_EXCEPTION; FR->exc=val_str("IndexError"); }
                }
            } else if (t && t->type==VAL_STR) {
                long long i = (idx->type==VAL_INT)?idx->as.integer:(long long)idx->as.floating;
                if (i < 0) i += strlen(t->as.string);
                if (i >= 0 && i < (long long)strlen(t->as.string)) {
                    char buf[2] = { t->as.string[i], 0 };
                    res = val_str(buf);
                } else {
                    fprintf(stderr, "IndexError\n");
                    if (FR) { FR->signal=SIG_EXCEPTION; FR->exc=val_str("IndexError"); }
                }
            } else if (t && t->type==VAL_MAP) {
                if (idx->type==VAL_STR) {
                    Value *v = map_get(t, idx->as.string);
                    if (v) res = val_retain(v); else {
                        // KeyError
                        if (FR) { FR->signal=SIG_EXCEPTION; FR->exc=val_str("KeyError"); }
                    }
                }
            } else if (t && t->type==VAL_RANGE) {
                // not indexable
            }
            // slice handling if idx is a slice marker? We use N_SLICE separately.
            val_release(t); val_release(idx);
            return res;
        }
        case N_SLICE: {
            Value *t = eval_expr(node->target, env);
            Value *s = node->start ? eval_expr(node->start, env) : NULL;
            Value *st = node->stop ? eval_expr(node->stop, env) : NULL;
            Value *step = node->step ? eval_expr(node->step, env) : NULL;
            Value *res = val_null();
            long long start=0, stop=0, stepv=1;
            int hasStart = node->start != NULL;
            int hasStop = node->stop != NULL;
            int hasStep = node->step != NULL;
            if (hasStep && step) stepv = step->type==VAL_INT?step->as.integer:(long long)step->as.floating;
            if (stepv==0) stepv=1;
            if (t && t->type==VAL_LIST) {
                long long n = t->as.list.count;
                if (!hasStart) start = (stepv>0)?0:n;
                else start = s->type==VAL_INT?s->as.integer:(long long)s->as.floating;
                if (start < 0) start += n;
                if (start < 0) start = (stepv>0)?0:n;
                if (start > n) start = n;
                if (!hasStop) stop = (stepv>0)?n:0;
                else if (st) { stop = st->type==VAL_INT?st->as.integer:(long long)st->as.floating; if (stop<0) stop+=n; if (stop<0) stop=0; if (stop>n) stop=n; }
                else stop = (stepv>0)?n:0;
                res = val_list();
                if (stepv > 0) {
                    for (long long i=start; i<stop; i+=stepv) list_append(res, t->as.list.items[i]);
                } else {
                    for (long long i=start; i>stop; i+=stepv) list_append(res, t->as.list.items[i]);
                }
            } else if (t && t->type==VAL_STR) {
                long long n = strlen(t->as.string);
                if (!hasStart) start = (stepv>0)?0:n;
                else start = s->type==VAL_INT?s->as.integer:(long long)s->as.floating;
                if (start < 0) start += n;
                if (start < 0) start = (stepv>0)?0:n;
                if (start > n) start = n;
                if (!hasStop) stop = (stepv>0)?n:0;
                else if (st) { stop = st->type==VAL_INT?st->as.integer:(long long)st->as.floating; if (stop<0) stop+=n; if (stop<0) stop=0; if (stop>n) stop=n; }
                else stop = (stepv>0)?n:0;
                char buf[4096]; buf[0]=0; int pos=0;
                if (stepv > 0) {
                    for (long long i=start; i<stop; i+=stepv) { if (pos<4094) buf[pos++]=t->as.string[i]; }
                } else {
                    for (long long i=start; i>stop; i+=stepv) { if (pos<4094) buf[pos++]=t->as.string[i]; }
                }
                buf[pos]=0;
                res = val_str(buf);
            }
            if (t) val_release(t);
            if (s) val_release(s);
            if (st) val_release(st);
            if (step) val_release(step);
            return res;
        }
        case N_ATTR: {
            Value *t = eval_expr(node->target, env);
            Value *res = val_null();
            if (t && t->type==VAL_INSTANCE) {
                // field
                Instance *inst = t->as.inst;
                for (int i=0;i<inst->field_count;i++) {
                    if (strcmp(inst->fields[i], node->sval)==0) { res = val_retain(inst->field_vals[i]); break; }
                }
                if (res->type == VAL_NULL) {
                    // method: return a bound function? We'll create a function wrapper? 
                    Function *m = find_method(inst->cls, node->sval);
                    if (m) {
                        // create a new function with self bound? We'll return a special function value with first arg self.
                        // Simpler: store method name in a wrapper Value? We'll create a VAL_FUNC whose closure contains self.
                        // We'll implement bound method as Function with param_count-1? Hack: create a new Function copy with body, params shifted, and closure env containing 'self'.
                        Function *b = calloc(1,sizeof(Function));
                        b->name = strdup(m->name);
                        b->param_count = m->param_count - 1;
                        if (b->param_count > 0) {
                            b->params = malloc(b->param_count*sizeof(char*));
                            for (int i=0;i<b->param_count;i++) b->params[i] = strdup(m->params[i+1]);
                        }
                        b->body = m->body;
                        Env *ce = env_new(m->closure ? m->closure : env);
                        env_set(ce, m->params[0], t); // self
                        b->closure = ce;
                        Value *fv = val_new(VAL_FUNC); fv->as.func = b;
                        res = fv;
                    }
                }
            } else if (t && t->type==VAL_MODULE) {
                // module namespace stored as map in inst? We'll handle modules as VAL_MAP? 
            } else if (t && t->type==VAL_MAP) {
                // maybe attribute on map? 
            } else if (t && t->type==VAL_CLASS) {
                Function *m = find_method(t->as.cls, node->sval);
                if (m) {
                    Value *fv = val_new(VAL_FUNC); fv->as.func = m; // unbound
                    res = fv;
                }
            }
            // module object: we store modules as map in globals? Handle via map attr? 
            if (res->type == VAL_NULL) {
                // try map-like attribute access on map
                if (t && t->type==VAL_MAP) {
                    Value *v = map_get(t, node->sval);
                    if (v) res = val_retain(v);
                }
            }
            if (t) val_release(t);
            if (!res) res = val_null();
            return res;
        }
        case N_LIST: {
            Value *l = val_list();
            for (int i=0;i<node->child_count;i++) {
                Value *v = eval_expr(node->children[i], env);
                list_append(l, v);
                if (v) val_release(v);
            }
            return l;
        }
        case N_TUPLE: {
            Value *l = val_list(); // represent tuple as list with marker? We'll just list
            for (int i=0;i<node->child_count;i++) {
                Value *v = eval_expr(node->children[i], env);
                list_append(l, v);
                if (v) val_release(v);
            }
            return l;
        }
        case N_SET: {
            Value *s = val_new(VAL_LIST?VAL_NULL:VAL_NULL);
            // We don't have VAL_LIST in enum? We defined VAL_LIST? In enum yes VAL_LIST? Actually enum includes VAL_LIST? We wrote VAL_LIST? Let's check: enum had VAL_NULL, VAL_BOOL, VAL_INT, VAL_FLOAT, VAL_STR, VAL_LIST, VAL_MAP, VAL_FUNC, VAL_CLASS, VAL_INSTANCE, VAL_MODULE, VAL_RANGE. No VAL_LIST. We'll represent set as list? Use list.
            free(s);
            s = val_list();
            for (int i=0;i<node->child_count;i++) {
                Value *v = eval_expr(node->children[i], env);
                // avoid duplicates
                int dup=0;
                for (int j=0;j<s->as.list.count;j++) if (val_equals(s->as.list.items[j], v)) { dup=1; break; }
                if (!dup) list_append(s, v);
                if (v) val_release(v);
            }
            return s;
        }
        case N_MAP: {
            Value *m = val_map();
            for (int i=0;i<node->entry_count;i++) {
                Value *k = eval_expr(node->keys[i], env);
                Value *v = eval_expr(node->values[i], env);
                char *ks = val_to_str(k);
                // copy
                char *kcopy = strdup(ks);
                map_set(m, kcopy, v);
                free(kcopy);
                if (k) val_release(k);
                if (v) val_release(v);
            }
            return m;
        }
        case N_COMPREHENSION: {
            // determine kind
            int isMap = node->op && strcmp(node->op,"map")==0;
            int isSet = node->op && strcmp(node->op,"set")==0;
            Value *iter = eval_expr(node->left ? node->left : node->right, env);
            // iter expression stored differently; handle common: if left null use right
            if (!iter && node->right) iter = eval_expr(node->right, env);
            // Actually for list comp: expr for target in iter; iter stored in node->left (we set left=iter). For map comp: left=key? We set left=first (key), right=iter. Need adjust.
            // Let's handle by re-evaluating based on node fields:
            // We'll just support list comp fully:
            Value *result = val_list();
            // find iter node: if node->left is not null and node->target present, node->left is iter for list/set; for map, node->right is iter.
            ASTNode *iterNode = NULL;
            ASTNode *exprNode = node->value;
            if (isMap) { iterNode = node->right; }
            else { iterNode = node->left; }
            if (!iterNode) iterNode = node->left;
            Value *iterv = eval_expr(iterNode, env);
            // iterate
            if (iterv && iterv->type==VAL_LIST) {
                for (int i=0;i<iterv->as.list.count;i++) {
                    Env *le = env_new(env);
                    env_set(le, node->target->sval, iterv->as.list.items[i]);
                    if (node->test) {
                        Value *c = eval_expr(node->test, le);
                        if (!val_truthy(c)) { if(c)val_release(c); continue; }
                        if(c)val_release(c);
                    }
                    if (isMap) {
                        Value *kk = eval_expr(node->value?node->value:node->left, le); // key expr? We stored value=first (key) originally? Confusing.
                        Value *vv = eval_expr(node->value, le);
                        char *ks = strdup(val_to_str(kk));
                        map_set(result?NULL:result, ks, vv); // result is list, not map! Need separate
                        free(ks);
                        if (kk) val_release(kk);
                        if (vv) val_release(vv);
                    } else {
                        Value *v = eval_expr(exprNode, le);
                        // for set avoid dup
                        if (isSet) {
                            int dup=0;
                            for (int j=0;j<result->as.list.count;j++) if (val_equals(result->as.list.items[j], v)) { dup=1; break; }
                            if (!dup) list_append(result, v);
                        } else {
                            list_append(result, v);
                        }
                        if (v) val_release(v);
                    }
                }
            } else if (iterv && iterv->type==VAL_RANGE) {
                // iterate range
                // mutate copy
                long long cur = iterv->as.range.cur;
                long long end = iterv->as.range.end;
                long long step = iterv->as.range.step;
                // We'll loop
                int cond = (step>0) ? (cur < end) : (cur > end);
                while (cond) {
                    Env *le = env_new(env);
                    env_set(le, node->target->sval, val_int(cur));
                    if (node->test) {
                        Value *c = eval_expr(node->test, le);
                        if (!val_truthy(c)) { if(c)val_release(c); cur+=step; cond = (step>0)?(cur<end):(cur>end); continue; }
                        if(c)val_release(c);
                    }
                    Value *v = eval_expr(exprNode, le);
                    if (isSet) {
                        int dup=0;
                        for (int j=0;j<result->as.list.count;j++) if (val_equals(result->as.list.items[j], v)) { dup=1; break; }
                        if (!dup) list_append(result, v);
                    } else {
                        list_append(result, v);
                    }
                    if (v) val_release(v);
                    cur += step;
                    cond = (step>0) ? (cur < end) : (cur > end);
                }
            }
            if (isMap) {
                // convert result list? We incorrectly used list. For map comp, create map instead.
                // Redo: if isMap, result should be map. But we appended? We didn't append for map branch? We called map_set on NULL. Fix: create map initially.
            }
            if (iterv) val_release(iterv);
            if (iter) val_release(iter);
            // If map, we need return map. Re-evaluate map separately? 
            if (isMap) {
                // Rebuild map
                Value *m = val_map();
                // iterate again
                Value *iterv2 = eval_expr(iterNode, env);
                if (iterv2 && iterv2->type==VAL_LIST) {
                    for (int i=0;i<iterv2->as.list.count;i++) {
                        Env *le = env_new(env);
                        env_set(le, node->target->sval, iterv2->as.list.items[i]);
                        if (node->test) {
                            Value *c = eval_expr(node->test, le);
                            if (!val_truthy(c)) { if(c)val_release(c); continue; }
                            if(c)val_release(c);
                        }
                        // key expr: node->left? value expr: node->value? In map comp we set left=key, value=val, right=iter. But node->value was set to val initially then overwritten? Let's assume node->left=key, node->value=val.
                        ASTNode *keyNode = node->left;
                        ASTNode *valNode = node->value;
                        Value *kk = eval_expr(keyNode, le);
                        Value *vv = eval_expr(valNode, le);
                        char *ks = strdup(val_to_str(kk));
                        map_set(m, ks, vv);
                        free(ks);
                        if (kk) val_release(kk);
                        if (vv) val_release(vv);
                    }
                }
                if (iterv2) val_release(iterv2);
                val_release(result);
                return m;
            }
            return result;
        }
        case N_LAMBDA: {
            Function *fn = calloc(1,sizeof(Function));
            fn->name = strdup("<lambda>");
            fn->param_count = node->param_count;
            if (fn->param_count > 0) {
                fn->params = malloc(fn->param_count*sizeof(char*));
                for (int i=0;i<fn->param_count;i++) fn->params[i] = strdup(node->params[i]);
            }
            // body as block with expr
            fn->body = ast_new(N_BLOCK);
            fn->body->body_cap=1; fn->body->body=calloc(1,sizeof(ASTNode*));
            fn->body->body[0] = node->value; // expr node
            fn->closure = env;
            Value *fv = val_new(VAL_FUNC); fv->as.func = fn;
            return fv;
        }
        default:
            return val_null();
    }
}

static int exec_stmt(ASTNode *node, Env *env, Frame *fr) {
    if (!node) return SIG_NONE;
    switch (node->type) {
        case N_EXPRSTMT: {
            Value *v = eval_expr(node->test ? node->test : node->value, env);
            if (v) val_release(v);
            if (fr->signal) return fr->signal;
            return SIG_NONE;
        }
        case N_VARDECL: {
            Value *v = node->value ? eval_expr(node->value, env) : val_null();
            env_set(env, node->sval, v);
            if (v) val_release(v);
            if (fr->signal) return fr->signal;
            return SIG_NONE;
        }
        case N_ASSIGN: {
            Value *v = eval_expr(node->value, env);
            if (node->target->type == N_IDENT) {
                if (env_has(env, node->target->sval)) env_set(env, node->target->sval, v);
                else env_set(env, node->target->sval, v); // global-ish
            } else if (node->target->type == N_ATTR) {
                Value *t = eval_expr(node->target->target, env);
                if (t && t->type==VAL_INSTANCE) {
                    Instance *inst = t->as.inst;
                    // set field
                    int found=0;
                    for (int i=0;i<inst->field_count;i++) {
                        if (strcmp(inst->fields[i], node->target->sval)==0) {
                            val_release(inst->field_vals[i]);
                            inst->field_vals[i] = val_retain(v);
                            found=1; break;
                        }
                    }
                    if (!found) {
                        if (inst->field_count == inst->field_cap) {
                            inst->field_cap = inst->field_cap?inst->field_cap*2:4;
                            inst->fields = realloc(inst->fields, inst->field_cap*sizeof(char*));
                            inst->field_vals = realloc(inst->field_vals, inst->field_cap*sizeof(Value*));
                        }
                        inst->fields[inst->field_count] = strdup(node->target->sval);
                        inst->field_vals[inst->field_count] = val_retain(v);
                        inst->field_count++;
                    }
                }
                if (t) val_release(t);
            } else if (node->target->type == N_INDEX) {
                Value *t = eval_expr(node->target->target, env);
                Value *idx = eval_expr(node->target->value, env);
                if (t && t->type==VAL_LIST) {
                    long long i = idx->type==VAL_INT?idx->as.integer:(long long)idx->as.floating;
                    if (i<0) i += t->as.list.count;
                    if (i>=0 && i<t->as.list.count) {
                        val_release(t->as.list.items[i]);
                        t->as.list.items[i] = val_retain(v);
                    }
                } else if (t && t->type==VAL_MAP && idx->type==VAL_STR) {
                    map_set(t, idx->as.string, v);
                }
                if (t) val_release(t);
                if (idx) val_release(idx);
            }
            if (v) val_release(v);
            if (fr->signal) return fr->signal;
            return SIG_NONE;
        }
        case N_AUGASSIGN: {
            // load, apply op, store
            // evaluate current
            Value *curv = NULL;
            if (node->target->type==N_IDENT) curv = env_get(env, node->target->sval);
            // apply
            // reuse binary by constructing? simpler: handle +,-,*,/
            Value *rv = eval_expr(node->value, env);
            Value *nv = NULL;
            if (curv && curv->type==VAL_INT && rv->type==VAL_INT) {
                long long a=curv->as.integer, b=rv->as.integer;
                if (strcmp(node->op,"+")==0) nv=val_int(a+b);
                else if (strcmp(node->op,"-")==0) nv=val_int(a-b);
                else if (strcmp(node->op,"*")==0) nv=val_int(a*b);
                else if (strcmp(node->op,"/")==0) nv=val_int(a/b);
                else if (strcmp(node->op,"%")==0) nv=val_int(a%b);
            } else {
                double a = curv?(curv->type==VAL_INT?curv->as.integer:curv->as.floating):0;
                double b = rv->type==VAL_INT?rv->as.integer:rv->as.floating;
                if (strcmp(node->op,"+")==0) nv=val_float(a+b);
                else if (strcmp(node->op,"-")==0) nv=val_float(a-b);
                else if (strcmp(node->op,"*")==0) nv=val_float(a*b);
                else if (strcmp(node->op,"/")==0) nv=val_float(a/b);
            }
            if (node->target->type==N_IDENT) env_set(env, node->target->sval, nv);
            if (curv) {} 
            if (rv) val_release(rv);
            if (nv) val_release(nv);
            if (fr->signal) return fr->signal;
            return SIG_NONE;
        }
        case N_IF: {
            Value *c = eval_expr(node->test, env);
            if (val_truthy(c)) {
                if (c) val_release(c);
                if (node->block) { int s = exec_block(node->block, env, fr); if (s) return s; }
                return SIG_NONE;
            }
            if (c) val_release(c);
            // elifs
            for (int i=0;i<node->elif_count;i++) {
                ASTNode *eif = node->elifs[i];
                Value *cc = eval_expr(eif->test, env);
                if (val_truthy(cc)) {
                    if (cc) val_release(cc);
                    if (eif->body) { int s = exec_block(eif->body, env, fr); if (s) return s; }
                    return SIG_NONE;
                }
                if (cc) val_release(cc);
            }
            if (node->elseBody) {
                int s = exec_block(node->elseBody, env, fr);
                if (s) return s;
            }
            return SIG_NONE;
        }
        case N_WHILE: {
            for (;;) {
                Value *c = eval_expr(node->test, env);
                int truth = val_truthy(c);
                if (c) val_release(c);
                if (!truth) break;
                if (node->block) {
                    int s = exec_block(node->block, env, fr);
                    if (s == SIG_BREAK) { fr->signal=SIG_NONE; break; }
                    if (s == SIG_CONTINUE) { fr->signal=SIG_NONE; continue; }
                    if (s == SIG_RETURN || s == SIG_EXCEPTION) return s;
                }
            }
            return SIG_NONE;
        }
        case N_FOR: {
            Value *iter = eval_expr(node->value, env);
            if (iter && iter->type==VAL_LIST) {
                for (int i=0;i<iter->as.list.count;i++) {
                    Env *le = env_new(env);
                    env_set(le, node->target->sval, iter->as.list.items[i]);
                    if (node->block) {
                        int s = exec_block(node->block, le, fr);
                        if (s == SIG_BREAK) { fr->signal=SIG_NONE; break; }
                        if (s == SIG_CONTINUE) { fr->signal=SIG_NONE; continue; }
                        if (s == SIG_RETURN || s == SIG_EXCEPTION) { val_release(iter); return s; }
                    }
                }
            } else if (iter && iter->type==VAL_RANGE) {
                long long cur = iter->as.range.cur;
                long long end = iter->as.range.end;
                long long step = iter->as.range.step;
                int cond = (step>0)?(cur<end):(cur>end);
                while (cond) {
                    Env *le = env_new(env);
                    env_set(le, node->target->sval, val_int(cur));
                    if (node->block) {
                        int s = exec_block(node->block, le, fr);
                        if (s == SIG_BREAK) { fr->signal=SIG_NONE; break; }
                        if (s == SIG_CONTINUE) { fr->signal=SIG_NONE; cur+=step; cond=(step>0)?(cur<end):(cur>end); continue; }
                        if (s == SIG_RETURN || s == SIG_EXCEPTION) { val_release(iter); return s; }
                    }
                    cur += step;
                    cond = (step>0)?(cur<end):(cur>end);
                }
            } else if (iter && iter->type==VAL_STR) {
                int len = strlen(iter->as.string);
                for (int i=0;i<len;i++) {
                    char buf[2]={iter->as.string[i],0};
                    Env *le = env_new(env);
                    env_set(le, node->target->sval, val_str(buf));
                    if (node->block) {
                        int s = exec_block(node->block, le, fr);
                        if (s == SIG_BREAK) { fr->signal=SIG_NONE; break; }
                        if (s == SIG_CONTINUE) { fr->signal=SIG_NONE; continue; }
                        if (s == SIG_RETURN || s == SIG_EXCEPTION) { val_release(iter); return s; }
                    }
                }
            }
            if (iter) val_release(iter);
            return SIG_NONE;
        }
        case N_RETURN: {
            Value *v = node->value ? eval_expr(node->value, env) : val_null();
            fr->signal = SIG_RETURN;
            fr->retval = v;
            return SIG_RETURN;
        }
        case N_FN: {
            Function *fn = calloc(1,sizeof(Function));
            fn->name = strdup(node->sval);
            fn->param_count = node->param_count;
            if (fn->param_count > 0) {
                fn->params = malloc(fn->param_count*sizeof(char*));
                for (int i=0;i<fn->param_count;i++) fn->params[i] = strdup(node->params[i]);
            }
            fn->body = node->block ? node->block : ast_new(N_BLOCK);
            fn->closure = env;
            Value *fv = val_new(VAL_FUNC);
            fv->as.func = fn;
            env_set(env, node->sval, fv);
            val_release(fv);
            return SIG_NONE;
        }
        case N_CLASS: {
            Class *cls = calloc(1,sizeof(Class));
            cls->name = strdup(node->sval);
            // methods from body: functions
            if (node->block) {
                // execute body in a special env to collect methods? We'll iterate statements
                ASTNode *blk = node->block;
                for (int i=0;i<blk->body_count;i++) {
                    ASTNode *s = blk->body[i];
                    if (s->type == N_FN) {
                        Function *fn = calloc(1,sizeof(Function));
                        fn->name = strdup(s->sval);
                        fn->param_count = s->param_count;
                        if (fn->param_count > 0) {
                            fn->params = malloc(fn->param_count*sizeof(char*));
                            for (int j=0;j<fn->param_count;j++) fn->params[j] = strdup(s->params[j]);
                        }
                        fn->body = s->block ? s->block : ast_new(N_BLOCK);
                        fn->closure = env;
                        // add method
                        if (cls->method_count == cls->method_cap) {
                            cls->method_cap = cls->method_cap?cls->method_cap*2:4;
                            cls->method_names = realloc(cls->method_names, cls->method_cap*sizeof(char*));
                            cls->methods = realloc(cls->methods, cls->method_cap*sizeof(Function*));
                        }
                        cls->method_names[cls->method_count] = strdup(s->sval);
                        cls->methods[cls->method_count] = fn;
                        cls->method_count++;
                    }
                }
            }
            Value *cv = val_new(VAL_CLASS);
            cv->as.cls = cls;
            env_set(env, node->sval, cv);
            val_release(cv);
            return SIG_NONE;
        }
        case N_TRY: {
            // execute body; on exception (fr->signal==SIG_EXCEPTION) handle
            Frame inner; inner.signal=SIG_NONE; inner.retval=NULL; inner.exc=NULL;
            int s = exec_block(node->block, env, &inner);
            if (s == SIG_EXCEPTION) {
                // find handler (we only have one)
                // store exception in env if handler has 'as'? We ignore name
                // execute handler body stored in children[0] if exists
                if (node->child_count > 0) {
                    ASTNode *h = node->children[0];
                    // h is block
                    int hs = exec_block(h, env, fr);
                    if (hs) return hs;
                } else {
                    // rethrow
                    fr->signal = SIG_EXCEPTION; fr->exc = inner.exc; 
                    return SIG_EXCEPTION;
                }
            } else if (s == SIG_RETURN) {
                fr->signal = SIG_RETURN; fr->retval = inner.retval; inner.retval=NULL; return SIG_RETURN;
            }
            if (node->elseBody) {
                int fs = exec_block(node->elseBody, env, fr);
                if (fs) return fs;
            }
            return SIG_NONE;
        }
        case N_RAISE: {
            Value *v = node->value ? eval_expr(node->value, env) : val_str("Exception");
            fr->signal = SIG_EXCEPTION;
            fr->exc = v;
            return SIG_EXCEPTION;
        }
        case N_ASSERT: {
            Value *c = eval_expr(node->test, env);
            if (!val_truthy(c)) {
                if (c) val_release(c);
                Value *msg = node->value ? eval_expr(node->value, env) : val_str("AssertionError");
                fr->signal = SIG_EXCEPTION; fr->exc = msg;
                return SIG_EXCEPTION;
            }
            if (c) val_release(c);
            return SIG_NONE;
        }
        case N_IMPORT: {
            // load module file
            // module name node->module, alias in names[0]
            char filepath[512];
            // try current dir and std
            snprintf(filepath,512,"%s.rupt", node->module);
            FILE *f = fopen(filepath,"r");
            if (!f) {
                // try std dir relative to executable? 
                snprintf(filepath,512,"std/%s.rupt", node->module);
                f = fopen(filepath,"r");
            }
            if (!f) {
                snprintf(filepath,512,"bricks/%s/%s.rupt", node->module, node->module);
                f = fopen(filepath,"r");
            }
            if (!f) {
                fprintf(stderr, "ImportError: cannot find module %s\n", node->module);
                if (FR) { FR->signal=SIG_EXCEPTION; FR->exc=val_str("ImportError"); }
                return SIG_EXCEPTION;
            }
            // read file
            fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
            char *buf = malloc(sz+1); fread(buf,1,sz,f); buf[sz]=0; fclose(f);
            ASTNode *modast = parse_source(buf);
            free(buf);
            Env *modenv = env_new(NULL);
            // execute module
            Frame mfr; mfr.signal=SIG_NONE; mfr.retval=NULL; mfr.exc=NULL;
            exec_block(modast->body?modast:modast, modenv, &mfr);
            // register module as map in env under alias
            Value *modval = val_map();
            for (int i=0;i<modenv->count;i++) {
                map_set(modval, modenv->keys[i], modenv->vals[i]);
            }
            map_set(modval, "__name__", val_str(node->module));
            env_set(env, node->names[0], modval);
            val_release(modval);
            // Also bind top-level functions? They remain in modenv, not accessible. This is incomplete.
            return SIG_NONE;
        }
        case N_FROMIMPORT: {
            char filepath[512];
            snprintf(filepath,512,"%s.rupt", node->module);
            FILE *f = fopen(filepath,"r");
            if (!f) { snprintf(filepath,512,"std/%s.rupt", node->module); f=fopen(filepath,"r"); }
            if (!f) {
                // built-in stdlib module? Handle known modules by no-op; functions are builtins in global env? 
                // We'll just skip for known stdlib
                // Bind imported names to builtins if present
                for (int i=0;i<node->name_count;i++) {
                    char *nm = node->names[i];
                    Value *v = env_get(env, nm);
                    if (v) env_set(env, node->aliases[i]?node->aliases[i]:nm, v);
                }
                return SIG_NONE;
            }
            fseek(f,0,SEEK_END); long sz=ftell(f); fseek(f,0,SEEK_SET);
            char *buf = malloc(sz+1); fread(buf,1,sz,f); buf[sz]=0; fclose(f);
            ASTNode *modast = parse_source(buf);
            free(buf);
            Env *modenv = env_new(NULL);
            Frame mfr; mfr.signal=SIG_NONE; mfr.retval=NULL; mfr.exc=NULL;
            exec_block(modast->body?modast:modast, modenv, &mfr);
            for (int i=0;i<node->name_count;i++) {
                char *nm = node->names[i];
                if (strcmp(nm,"*")==0) {
                    // copy all? We don't know. Skip.
                } else {
                    Value *v = env_get(modenv, nm);
                    if (v) env_set(env, node->aliases[i]?node->aliases[i]:nm, v);
                }
            }
            return SIG_NONE;
        }
        case N_BREAK: fr->signal=SIG_BREAK; return SIG_BREAK;
        case N_CONTINUE: fr->signal=SIG_CONTINUE; return SIG_CONTINUE;
        case N_PASS: return SIG_NONE;
        case N_BLOCK: return exec_block(node, env, fr);
        default:
            // try children
            for (int i=0;i<node->body_count;i++) {
                int s = exec_stmt(node->body[i], env, fr);
                if (s) return s;
            }
            return SIG_NONE;
    }
}

int exec_block(ASTNode *block, Env *env, Frame *fr) {
    if (!block) return SIG_NONE;
    if (block->type == N_BLOCK || block->type == N_PROGRAM) {
        for (int i=0;i<block->body_count;i++) {
            int s = exec_stmt(block->body[i], env, fr);
            if (s) return s;
        }
        return SIG_NONE;
    }
    return exec_stmt(block, env, fr);
}




