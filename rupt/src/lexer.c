#include "rupt.h"
#include <ctype.h>

static void addtok(Lexer *L, TType type, const char *text, double num, int line, int col) {
    if (L->count == L->cap) { L->cap = L->cap? L->cap*2:1024; L->toks = realloc(L->toks, L->cap*sizeof(Token)); }
    L->toks[L->count].type=type;
    L->toks[L->count].text = text? strdup(text):NULL;
    L->toks[L->count].num=num;
    L->toks[L->count].line=line;
    L->toks[L->count].col=col;
    L->count++;
}

static int iskw(const char *s) {
    const char *kws[] = {"fn","def","class","if","elif","else","for","while","return","import","from","as","try","except","finally","raise","async","await","match","case","true","false","none","and","or","not","in","is","with","yield","lambda","test","assert","break","continue","pass","del","global","nonlocal","rupt",NULL};
    for (int i=0;kws[i];i++) if (strcmp(kws[i],s)==0) return 1;
    return 0;
}

Lexer *lex_source(const char *src) {
    Lexer *L = calloc(1,sizeof(Lexer));
    int n = strlen(src);
    int pos=0,line=1,col=1;
    int atLineStart=1;
    int indentStack[256]; int indentTop=0; indentStack[0]=0;
    while (pos < n) {
        char ch = src[pos];
        if (ch == '\n') {
            addtok(L, T_NEWLINE, NULL, 0, line, col);
            pos++; line++; col=1; atLineStart=1; continue;
        }
        if (ch == '\r') { pos++; continue; }
        if (atLineStart) {
            if (ch == ' ' || ch == '\t') {
                int spaces=0; int p=pos;
                while (p<n && (src[p]==' '||src[p]=='\t')) { if(src[p]=='\t') spaces+=4; else spaces++; p++; }
                if (p>=n || src[p]=='\n' || src[p]=='\r' || src[p]=='#') {
                    pos=p; continue;
                }
                int cur = indentStack[indentTop];
                if (spaces > cur) {
                    indentStack[++indentTop]=spaces;
                    addtok(L, T_INDENT, NULL, spaces, line, 1);
                } else if (spaces < cur) {
                    while (indentTop>0 && indentStack[indentTop] > spaces) {
                        indentTop--;
                        addtok(L, T_DEDENT, NULL, 0, line, 1);
                    }
                }
                pos=p; col=spaces+1; atLineStart=0; continue;
            }
            if (indentStack[indentTop] != 0) {
                while (indentTop>0) { indentTop--; addtok(L,T_DEDENT,NULL,0,line,1); }
            }
            atLineStart=0;
        }
        if (ch == '#') { while (pos<n && src[pos]!='\n') pos++; continue; }
        if (ch == ' ' || ch == '\t') { pos++; col++; continue; }
        int sl=line, sc=col;
        if (isdigit((unsigned char)ch)) {
            char buf[256]; int bi=0; int hasDot=0, hasExp=0;
            while (pos<n && (isdigit((unsigned char)src[pos]) || src[pos]=='.' || src[pos]=='_' || src[pos]=='e' || src[pos]=='E' || src[pos]=='+' || src[pos]=='-')) {
                if (src[pos]=='_') { pos++; col++; continue; }
                if (bi<255) buf[bi++]=src[pos];
                if (src[pos]=='.') hasDot=1;
                if (src[pos]=='e'||src[pos]=='E') hasExp=1;
                pos++; col++;
            }
            buf[bi]=0;
            double val = hasDot||hasExp ? atof(buf) : (double)atoll(buf);
            addtok(L, T_NUMBER, buf, val, sl, sc);
            continue;
        }
        if (ch == '"' || ch == '\'') {
            char quote=ch; pos++; col++;
            char buf[4096]; int bi=0;
            while (pos<n && src[pos]!=quote) {
                if (src[pos]=='\\') {
                    pos++; col++;
                    char esc=src[pos];
                    char m=esc;
                    if (esc=='n') m='\n'; else if (esc=='t') m='\t'; else if (esc=='r') m='\r';
                    if (bi<4095) buf[bi++]=m;
                    pos++; col++; continue;
                }
                if (src[pos]=='\n') { line++; col=1; }
                else col++;
                if (bi<4095) buf[bi++]=src[pos];
                pos++;
            }
            buf[bi]=0;
            pos++; col++;
            addtok(L, T_STRING, buf, 0, sl, sc);
            continue;
        }
        char two[4]={src[pos],src[pos+1],0,0};
        char three[5]={src[pos],src[pos+1],src[pos+2],0,0};
        if (strcmp(three,"**=")==0 || strcmp(three,"//=")==0) {
            addtok(L,T_OP,three,0,sl,sc); pos+=3; col+=3; continue;
        }
        const char *ops2[] = {"==","!=","<=",">=","**","//","+=","-=","*=","/=","%=","->","=>",NULL};
        int is2=0; for(int i=0;ops2[i];i++) if(strncmp(ops2[i],two,2)==0){is2=1;break;}
        if (is2) { addtok(L,T_OP,two,0,sl,sc); pos+=2; col+=2; continue; }
        if (strchr("+-*/%<>=!&|",ch)) { char b[2]={ch,0}; addtok(L,T_OP,b,0,sl,sc); pos++; col++; continue; }
        if (strchr("()[]{}:,.@",ch)) { char b[2]={ch,0}; addtok(L,T_PUNCT,b,0,sl,sc); pos++; col++; continue; }
        if (isalpha((unsigned char)ch) || ch=='_') {
            char buf[256]; int bi=0;
            while (pos<n && (isalnum((unsigned char)src[pos])||src[pos]=='_')) {
                if (bi<255) buf[bi++]=src[pos];
                pos++; col++;
            }
            buf[bi]=0;
            if (iskw(buf)) addtok(L,T_KEYWORD,buf,0,sl,sc);
            else addtok(L,T_IDENT,buf,0,sl,sc);
            continue;
        }
        char b[2]={ch,0}; addtok(L,T_UNKNOWN,b,0,sl,sc); pos++; col++;
    }
    while (indentTop>0) { indentTop--; addtok(L,T_DEDENT,NULL,0,line,1); }
    addtok(L,T_EOF,NULL,0,line,col);
    return L;
}
