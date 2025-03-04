#include "input.h"
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include "strtab.h"

enum {
    TOK_KW_META,    /* .META */
    TOK_KW_END,     /* .END */
    TOK_KW_LIST,    /* .LIST */
    TOK_KW_DELIM,   /* .DELIM */
    TOK_KW_EMPTY,   /* .EMPTY */
    TOK_KW_NUM,     /* .NUM */
    TOK_KW_ID,      /* .ID */
    TOK_KW_OCT,     /* .OCT */
    TOK_KW_HEX,     /* .HEX */
    TOK_KW_SR,      /* .SR */
    TOK_KW_CHR,     /* .CHR */
    TOK_KW_DIG,     /* .DIG */
    TOK_KW_LET,     /* .LET */
    TOK_ID,         /* identifier */
    TOK_INT,        /* integer */
    TOK_STR,        /* string */
    TOK_COMMA,      /* , */
    TOK_SEMI,       /* ; */
    TOK_COLON,      /* : */
    TOK_DOT,        /* . */
    TOK_AT,         /* @ */
    TOK_EXCL,       /* ! */
    TOK_DOLLAR,     /* $ */
    TOK_ASN,        /* := */
    TOK_BACKARROW,  /* <- */
    TOK_ARROW,      /* => */
    TOK_QMARK,      /* ? */
    TOK_PLUS,       /* + */
    TOK_MINUS,      /* - */
    TOK_CARET,      /* ^ */
    TOK_TILDE,      /* ~ */
    TOK_LPAREN,     /* ( */
    TOK_RPAREN,     /* ) */
    TOK_LBRACKET,   /* [ */
    TOK_RBRACKET,   /* ] */
    TOK_AND,        /* & */
    TOK_OR,         /* | */
    TOK_LSHIFT,     /* << */
    TOK_RSHIFT,     /* >> */
    TOK_RSHIFT2,    /* >>> */
    TOK_EOF,
    /* maintain order of what follows */
    TOK_STAR,       /* * */
    TOK_SLASH,      /* / */
    TOK_SLASH2,     /* // */
    TOK_PERC,       /* % */
    TOK_PERC2,      /* %% */
    TOK_EQ,         /* = */
    TOK_HASH,       /* # */
    TOK_GREATER,    /* > */
    TOK_LESS,       /* < */
    TOK_GREATEQ,    /* >= */
    TOK_LESSEQ,     /* <= */
};

static char *input_path;
static char *curr;
static char token_string[MAX_TOKSTR_LEN];
static int line_counter = 1;
static int LA;

static void input_error(char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "%s: %s:%d: error: ", prog_name, input_path, line_counter);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

static void skip_white(void)
{
    int done;

    done = FALSE;
    while (!done) {
        done = TRUE;
        if (isspace(*curr)) {
            do {
                if (*curr == '\n')
                    ++line_counter;
                ++curr;
            } while (isspace(*curr));
            done = FALSE;
        }
        if (curr[0]=='/' && curr[1]=='*') {
            int sl;

            sl = line_counter;
            curr += 2;
            while (curr[0]!='\0' && (curr[0]!='*' || curr[1]!='/'))
                if (*curr++ == '\n')
                    ++line_counter;
            if (*curr == '\0')
                input_error("unterminated comment (starts at line %d)", sl);
            curr += 2;
            done = FALSE;
        }
    }
}

static int get_token(void)
{
    char *p;

    for (;;) {
        skip_white();
        p = token_string;
        if (*curr == '.') {
            *p++ = *curr++;
            if (isalpha(*curr)) {
                int n;
#define TEST_KW(s)                              \
    if (strncmp(curr, #s, n=strlen(#s)) == 0) { \
        strncpy(p, curr, n);                    \
        p[n] = '\0';                            \
        curr += n;                              \
        return TOK_KW_##s;                      \
    }
                TEST_KW(META);
                TEST_KW(END);
                TEST_KW(LIST);
                TEST_KW(DELIM);
                TEST_KW(EMPTY);
                TEST_KW(NUM);
                TEST_KW(ID);
                TEST_KW(OCT);
                TEST_KW(HEX);
                TEST_KW(SR);
                TEST_KW(CHR);
                TEST_KW(DIG);
                TEST_KW(LET);
#undef TEST_KW
            }
            *p = '\0';
            return TOK_DOT;
        } else if (isalpha(*curr)) {
            do
                *p++ = *curr++;
            while (isalnum(*curr));
            *p = '\0';
            return TOK_ID;
        } else if (isdigit(*curr)) {
            do
                *p++ = *curr++;
            while (isdigit(*curr));
            *p = '\0';
            return TOK_INT;
        } else if (*curr == '\'') { /* XXX: .DELIM is supposed to affect this */
            ++curr;
            while (*curr!='\0' && *curr!='\'' && *curr!='\n')
                *p++ = *curr++;
            if (*curr++ != '\'')
                input_error("unterminated string literal");
            *p = '\0';
            return TOK_STR;
        } else {
            switch (*p++ = *curr++) {
            case '\0': --curr; return TOK_EOF;
            case ',': *p = '\0'; return TOK_COMMA;
            case ';': *p = '\0'; return TOK_SEMI;
            case '.': *p = '\0'; return TOK_DOT;
            case '@': *p = '\0'; return TOK_AT;
            case '!': *p = '\0'; return TOK_EXCL;
            case '$': *p = '\0'; return TOK_DOLLAR;
            case '\?': *p = '\0'; return TOK_QMARK;
            case '+': *p = '\0'; return TOK_PLUS;
            case '-': *p = '\0'; return TOK_MINUS;
            case '*': *p = '\0'; return TOK_STAR;
            case '^': *p = '\0'; return TOK_CARET;
            case '~': *p = '\0'; return TOK_TILDE;
            case '(': *p = '\0'; return TOK_LPAREN;
            case ')': *p = '\0'; return TOK_RPAREN;
            case '[': *p = '\0'; return TOK_LBRACKET;
            case ']': *p = '\0'; return TOK_RBRACKET;
            case '#': *p = '\0'; return TOK_HASH;
            case '&': *p = '\0'; return TOK_AND;
            case '|': *p = '\0'; return TOK_OR;
            case '%':
                if (*curr == '%') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_PERC2;
                }
                *p = '\0';
                return TOK_PERC;
            case '/':
                if (*curr == '/') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_SLASH2;
                }
                *p = '\0';
                return TOK_SLASH;
            case '>':
                if (*curr == '=') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_GREATEQ;
                } else if (*curr == '>') {
                    *p++ = *curr++;
                    if (*curr == '>') {
                        *p++ = *curr++;
                        *p = '\0';
                        return TOK_RSHIFT2;
                    } else {
                        *p = '\0';
                        return TOK_RSHIFT;
                    }
                }
                *p = '\0';
                return TOK_GREATER;
            case '<':
                if (*curr == '-') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_BACKARROW;
                } else if (*curr == '<') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_LSHIFT;
                } else if (*curr == '=') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_LESSEQ;
                }
                *p = '\0';
                return TOK_LESS;
            case ':':
                if (*curr == '=') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_ASN;
                }
                *p = '\0';
                return TOK_COLON;
            case '=':
                if (*curr == '>') {
                    *p++ = *curr++;
                    *p = '\0';
                    return TOK_ARROW;
                }
                *p = '\0';
                return TOK_EQ;
            default:
                ++curr;
                continue;
            }
        }
    }
    assert(0);
}

static void match(int expected)
{
    if (LA == expected)
        LA = get_token();
    else
        input_error("unexpected `%s'", token_string);
}

SyntaxRule *syntax_rules;
static int syntax_nundef;

static SRNode *new_srnode(OpKind op)
{
    SRNode *n;

    n = calloc(1, sizeof(*n));
    n->op = op;
    return n;
}

static SyntaxRule *lookup_syntax_rule(char *name, SRNode *def)
{
    SyntaxRule *r;

    for (r = syntax_rules; r != NULL; r = r->next)
        if (strcmp(r->name, name) == 0)
            break;
    if (r == NULL) {
        r = malloc(sizeof(*r));
        r->name = strtab_new(name, strlen(name));
        if ((r->def=def) == NULL)
            ++syntax_nundef;
        r->next = syntax_rules;
        syntax_rules = r;
    } else if (r->def == NULL) {
        if ((r->def=def) != NULL)
            --syntax_nundef;
    } else if (def != NULL) {
        input_error("syntax rule `%s' redefined", name);
    }
    return r;
}

CodeRule *code_rules;
static int code_nundef;

static OutExpr *new_outexpr(OutExprKind kind)
{
    OutExpr *n;

    n = calloc(1, sizeof(*n));
    n->kind = kind;
    return n;
}

static CodeRule *lookup_code_rule(char *name, unsigned attr, OutRule *def)
{
    CodeRule *r;

    for (r = code_rules; r != NULL; r = r->next)
        if (strcmp(r->name, name) == 0)
            break;
    if (r == NULL) {
        r = malloc(sizeof(*r));
        r->name = strtab_new(name, strlen(name));
        if ((r->def=def) == NULL)
            ++code_nundef;
        else
            r->attr = attr;
        r->next = code_rules;
        code_rules = r;
    } else if (r->def == NULL) {
        if ((r->def=def) != NULL) {
            r->attr = attr;
            --code_nundef;
        }
    } else if (def != NULL) {
        input_error("code rule `%s' redefined", name);
    }
    return r;
}

/* ========================================================================== */
/* SYMBOL TABLE                                                               */
/* ========================================================================== */

static OutExpr *outexpression(void);

/* symbolrule = ID ":=" outexpression ";" */
static void symbolrule(void)
{
    OutRule *def;
    char rname[MAX_TOKSTR_LEN];

    if (LA == TOK_ID)
        strcpy(rname, token_string);
    match(TOK_ID);
    match(TOK_ASN);
    def = calloc(1, sizeof(*def));
    def->out_expr = outexpression();
    (void)lookup_code_rule(rname, CR_ATTR_SYMBOL, def);
    match(TOK_SEMI);
}

/* ========================================================================== */
/* ARITHMETIC STATEMENTS                                                      */
/* ========================================================================== */

static NodeName *nodename(void);

typedef enum {
    ARG_NODENAME,
    ARG_EXPRESSION,
} ArgKind;

static struct {
    Function func;
    char *name;
    ArgKind arg_kind;
} functions[] = {
    { F_LEN,    "LEN",      ARG_NODENAME   },
    { F_CODE,   "CODE",     ARG_NODENAME   },
    { F_CONV,   "CONV",     ARG_NODENAME   },
    { F_XCONV,  "XCONV",    ARG_NODENAME   },
    { F_POP,    "POP",      ARG_EXPRESSION },
    { F_PUSH,   "PUSH",     ARG_EXPRESSION },
    { F_OUT,    "OUT",      ARG_EXPRESSION },
    { F_OUTS,   "OUTS",     ARG_EXPRESSION },
    { F_OUTL,   "OUTL",     ARG_NODENAME   },
    { F_OUTC,   "OUTC",     ARG_NODENAME   },
    { F_ENTER,  "ENTER",    ARG_NODENAME   },
    { F_LOOK,   "LOOK",     ARG_NODENAME   },
    { F_CLEAR,  "CLEAR",    ARG_EXPRESSION },
    { -1,       NULL,       -1             },
};

static ArithExpr *new_arithexpr(ArithKind kind)
{
    ArithExpr *n;

    n = calloc(1, sizeof(*n));
    n->kind = kind;
    return n;
}

typedef struct Variable Variable;
static struct Variable {
    char *name;
    ArithType cell;
    Variable *next;
} *variables;

ArithType *TYPE_var, *LEVEL_var, *VALUE_var;

static ArithType *get_variable(char *name)
{
    Variable *var;

    for (var = variables; var != NULL; var = var->next)
        if (strcmp(var->name, name) == 0)
            return &var->cell;
    var = malloc(sizeof(*var));
    var->name = strtab_new(name, strlen(name));
    var->cell = 0;
    var->next = variables;
    variables = var;
    return &var->cell;
}

static ArithExpr *expression(void);

/* function = ID "[" ( nodename | expression ) "]" */
static ArithExpr *function(void)
{
    int i;
    ArithExpr *n;

    if (LA == TOK_ID) {
        for (i = 0; functions[i].name != NULL; i++)
            if (strcmp(token_string, functions[i].name) == 0)
                break;
        if (functions[i].name == NULL)
            input_error("unknown function `%s'", token_string);
        n = new_arithexpr(A_FUNC);
        n->func = functions[i].func;
    }
    match(TOK_ID);
    match(TOK_LBRACKET);
    if (functions[i].arg_kind == ARG_NODENAME)
        n->arg.nodnam = nodename();
    else /*if (functions[i].arg_kind == ARG_EXPRESSION)*/
        n->arg.expr = expression();
    match(TOK_RBRACKET);
    return n;
}

/*
    primary_expr = ID
                 | INT
                 | function
                 | "(" expression ")"
*/
static ArithExpr *primary_expr(void)
{
    ArithExpr *n;

    switch (LA) {
    case TOK_INT:
        n = new_arithexpr(A_INT);
        errno = 0;
        n->val = (ArithType)strtoul(token_string, NULL, 10);
        if (errno == ERANGE)
            input_error("number out of range");
        match(TOK_INT);
        break;
    case TOK_LPAREN:
        match(TOK_LPAREN);
        n = expression();
        match(TOK_RPAREN);
        break;
    case TOK_ID:
        skip_white();
        if (*curr == '[') {
            n = function();
        } else {
            n = new_arithexpr(A_VAR);
            n->var = get_variable(token_string);
            match(TOK_ID);
        }
        break;
    default:
        match(-1);
        break;
    }
    return n;
}

/* unary_expr = [ "+" | "-" | "~" | "!" ] primary_expr */
static ArithExpr *unary_expr(void)
{
    ArithExpr *n;

    switch (LA) {
    case TOK_MINUS:
        match(TOK_MINUS);
        n = new_arithexpr(A_NEG);
        n->child[0] = primary_expr();
        break;
    case TOK_EXCL:
        match(TOK_EXCL);
        n = new_arithexpr(A_LNEG);
        n->child[0] = primary_expr();
        break;
    case TOK_TILDE:
        match(TOK_TILDE);
        n = new_arithexpr(A_COMPL);
        n->child[0] = primary_expr();
        break;
    case TOK_PLUS:
        match(TOK_PLUS);
    default:
        n = primary_expr();
        break;
    }
    return n;
}

/* mult_expr = unary_expr { ( "*" | "/" | "//" | "%" | "%%" ) unary_expr } */
static ArithExpr *mult_expr(void)
{
    ArithExpr *n, *t;

    n = unary_expr();
    while (LA>=TOK_STAR && LA<=TOK_PERC2) {
        t = new_arithexpr(A_MUL+LA-TOK_STAR);
        match(LA);
        t->child[0] = n;
        t->child[1] = unary_expr();
        n = t;
    }
    return n;
}

/* add_expr = mult_expr { ( "+" | "-" ) mult_expr } */
static ArithExpr *add_expr(void)
{
    ArithExpr *n, *t;

    n = mult_expr();
    while (LA==TOK_PLUS || LA==TOK_MINUS) {
        t = new_arithexpr(LA==TOK_PLUS?A_ADD:A_SUB);
        match(LA);
        t->child[0] = n;
        t->child[1] = mult_expr();
        n = t;
    }
    return n;
}

/* shift_expr = add_expr { ( "<<" | ">>" | ">>>" ) add_expr } */
static ArithExpr *shift_expr(void)
{
    ArithExpr *n, *t;

    n = add_expr();
    while (LA==TOK_LSHIFT || LA==TOK_RSHIFT || LA==TOK_RSHIFT2) {
        t = new_arithexpr(LA==TOK_LSHIFT?A_LSH:(LA==TOK_RSHIFT)?A_URSH:A_SRSH);
        match(LA);
        t->child[0] = n;
        t->child[1] = add_expr();
        n = t;
    }
    return n;
}

/* AND_expr = shift_expr { "&" shift_expr } */
static ArithExpr *AND_expr(void)
{
    ArithExpr *n, *t;

    n = shift_expr();
    while (LA == TOK_AND) {
        t = new_arithexpr(A_AND);
        match(LA);
        t->child[0] = n;
        t->child[1] = shift_expr();
        n = t;
    }
    return n;
}

/* excl_OR_expr = AND_expr { "^" AND_expr } */
static ArithExpr *excl_OR_expr(void)
{
    ArithExpr *n, *t;

    n = AND_expr();
    while (LA == TOK_CARET) {
        t = new_arithexpr(A_EOR);
        match(LA);
        t->child[0] = n;
        t->child[1] = AND_expr();
        n = t;
    }
    return n;
}

/* expression = excl_OR_expr { "|" excl_OR_expr } */
ArithExpr *expression(void)
{
    ArithExpr *n, *t;

    n = excl_OR_expr();
    while (LA == TOK_OR) {
        t = new_arithexpr(A_OR);
        match(LA);
        t->child[0] = n;
        t->child[1] = excl_OR_expr();
        n = t;
    }
    return n;
}

/* relation = ID ( "=" | "#" | ">" | "<" | ">=" | "<=" ) [ "/" "+" ] expression */
static ArithExpr *relation(void)
{
    ArithExpr *n;

    if (LA == TOK_ID) {
        n = new_arithexpr(-1);
        n->lhs = get_variable(token_string);
    }
    match(TOK_ID);
    if (LA>=TOK_EQ && LA<=TOK_LESSEQ) {
        n->kind = A_EQ+LA-TOK_EQ;
        match(LA);
    } else {
        match(-1);
    }
    if (LA == TOK_SLASH) {
        match(TOK_SLASH);
        match(TOK_PLUS);
        switch (n->kind) {
        case A_UGT: n->kind = A_SGT; break;
        case A_ULT: n->kind = A_SLT; break;
        case A_UGET: n->kind = A_SGET; break;
        case A_ULET: n->kind = A_SLET; break;
        }
    }
    n->rhs = expression();
    return n;
}

/* assignment = ID "<-" expression */
static ArithExpr *assignment(void)
{
    ArithExpr *n;

    if (LA == TOK_ID) {
        n = new_arithexpr(A_ASN);
        n->lhs = get_variable(token_string);
    }
    match(TOK_ID);
    match(TOK_BACKARROW);
    n->rhs = expression();
    return n;
}

/*
    arith = assignment
          | function
          | relation
*/
static ArithExpr *arith(void)
{
    skip_white();
    if (*curr == '<')
        return assignment();
    if (*curr == '[')
        return function();
    return relation();
}

/* arithlist =  arith { ";" arith } */
static ArithExpr *arithlist(void)
{
    ArithExpr *n, *t;

    t = n = arith();
    while (LA == TOK_SEMI) {
        match(TOK_SEMI);
        t->next = arith();
        t = t->next;
    }
    return n;
}

/* arithmetic = "<" arithlist ">" */
static ArithExpr *arithmetic(void)
{
    ArithExpr *n;

    match(TOK_LESS);
    n = arithlist();
    match(TOK_GREATER);
    return n;
}

/* ========================================================================== */
/* CODE RULES                                                                 */
/* ========================================================================== */

static SRNode *basictype(void);
static NodeTestItem *nodetest(void);

/* label = "#" INT */
static int label(void)
{
    int slot;

    match(TOK_HASH);
    if (LA == TOK_INT) {
        slot = atoi(token_string);
        if (slot<1 || slot>4)
            input_error("invalid label number");
    }
    match(TOK_INT);
    return slot-1; /* return array index */
}

/*
    outputtext = "%"
               | STR
               | "!" STR
               | "@" INT
*/
static OutExpr *outputtext(void)
{
    OutExpr *n;

    n = calloc(1, sizeof(*n));
    switch (LA) {
    case TOK_PERC:
        match(TOK_PERC);
        n->kind = OE_NEWLINE;
        break;
    case TOK_STR:
        n->text = strtab_new(token_string, strlen(token_string));
        match(TOK_STR);
        n->kind = OE_TEXT;
        break;
    case TOK_EXCL:
        match(TOK_EXCL);
        // XXX: embed assembly code in the generated compiler
        match(TOK_STR);
        break;
    case TOK_AT:
        match(TOK_AT);
        if (LA == TOK_INT)
            n->chrcode = atoi(token_string);
        match(TOK_INT);
        n->kind = OE_CHRCODE;
        break;
    default:
        match(-1);
        break;
    }
    return n;
}

/* simpleoutexpression = outputtext { outputtext } */
static OutExpr *simpleoutexpression(void)
{
    OutExpr *n, *t;

    n = outputtext();
    while (LA != TOK_SEMI) {
        t = new_outexpr(OE_SEQ);
        t->child[0] = n;
        t->child[1] = outputtext();
        n = t;
    }
    return n;
}

/*
    simplecoderule = ID "/" "=>" simpleoutexpression
                   | ID "/" "=>" ".EMPTY"
*/
static void simplecoderule(void)
{
    OutRule *def;
    char rname[MAX_TOKSTR_LEN];

    if (LA == TOK_ID)
        strcpy(rname, token_string);
    match(TOK_ID);
    match(TOK_SLASH);
    match(TOK_ARROW);
    def = calloc(1, sizeof(*def));
    if (LA == TOK_KW_EMPTY) {
        def->out_expr = new_outexpr(OE_EMPTY);
        match(TOK_KW_EMPTY);
    } else {
        def->out_expr = simpleoutexpression();
    }
    (void)lookup_code_rule(rname, CR_ATTR_SIMPLE, def);
}

typedef struct CRArg CRArg;
struct CRArg {
    enum {
        CR_ARG_NODENAME,
        CR_ARG_LABEL,
        CR_ARG_TEXT,
    } kind;
    union {
        NodeName *nodnam;
        int labslot;
        char *text;
    };
    CRArg *next;
};

/*
    argmnt = nodename
           | label
           | STR
*/
static CRArg *argmnt(void)
{
    CRArg *n;

    n = calloc(1, sizeof(*n));
    switch (LA) {
    case TOK_STAR:
        n->kind = CR_ARG_NODENAME;
        n->nodnam = nodename();
        break;
    case TOK_STR:
        n->kind = CR_ARG_TEXT;
        n->text = strtab_new(token_string, strlen(token_string));
        match(TOK_STR);
        break;
    case TOK_HASH:
        n->kind = CR_ARG_LABEL;
        n->labslot = label();
        break;
    default:
        match(-1);
        break;
    }
    return n;
}

/* arglist = argmnt { "," argmnt } */
static CRArg *arglist(int *narg)
{
    CRArg *n, *t;

    t = n = argmnt();
    *narg = 1;
    while (LA == TOK_COMMA) {
        match(TOK_COMMA);
        t->next = argmnt();
        t = t->next;
        ++*narg;
    }
    return n;
}

/* nodename = "*" INT { ":" "*" INT } */
NodeName *nodename(void)
{
    NodeName n, *t;

    t = &n;
    goto first;
    while (LA == TOK_COLON) {
        match(TOK_COLON);
    first:
        match(TOK_STAR);
        if (LA == TOK_INT) {
            t->next = malloc(sizeof(*t->next));
            t = t->next;
            t->branch = atoi(token_string);
            t->next = NULL;
        }
        match(TOK_INT);
    }
    return n.next;
}

/*
    outitem = outputtext
            | nodename
            | ID "[" [ arglist ] "]"
            | arithmetic
            | "(" outexpression ")"
            | ".EMPTY"
            | label
*/
static OutExpr *outitem(void)
{
    OutExpr *n;

    switch (LA) {
    case TOK_PERC: case TOK_STR: case TOK_EXCL: case TOK_AT:
        n = outputtext();
        break;
    case TOK_STAR:
        n = new_outexpr(OE_NODENAME);
        n->nodnam = nodename();
        break;
    case TOK_ID: {
        TreeNode *t;

        n = new_outexpr(OE_CRCALL);
        t = calloc(1, sizeof(*t));
        t->node.name = lookup_code_rule(token_string, 0, NULL)->name;
        match(TOK_ID);
        match(TOK_LBRACKET);
        if (LA != TOK_RBRACKET) {
            int i;
            CRArg *a, *b;
            TreeNode *q, h;

            a = arglist(&t->node.nbr);
            q = &h;
            for (i = 0; a != NULL; i++, a=b) {
                q->sibling = calloc(1, sizeof(*q->sibling));
                q = q->sibling;
                switch (a->kind) {
                case CR_ARG_NODENAME:
                    q->node.name = (char *)a->nodnam;
                    n->cr.nodnam_arg = TRUE;
                    break;
                case CR_ARG_TEXT:
                    q->isleaf = TRUE;
                    q->str.type = OP_TEXT;
                    q->str.text = a->text;
                    break;
                case CR_ARG_LABEL:
                    q->isleaf = TRUE;
                    q->str.type = OP_LAB;
                    q->str.labslot = a->labslot;
                    n->cr.lab_arg = TRUE;
                    break;
                }
                b = a->next;
                free(a);
            }
            t->node.child = h.sibling;
        }
        n->cr.tree = t;
        match(TOK_RBRACKET);
    }
        break;
    case TOK_LESS:
        n = new_outexpr(OE_ARITH);
        n->expr = arithmetic();
        break;
    case TOK_LPAREN:
        match(TOK_LPAREN);
        n = outexpression();
        match(TOK_RPAREN);
        break;
    case TOK_KW_EMPTY:
        n = new_outexpr(OE_EMPTY);
        match(TOK_KW_EMPTY);
        break;
    case TOK_HASH:
        n = new_outexpr(OE_LABEL);
        n->labslot = label();
        break;
    default:
        match(-1);
        break;
    }
    return n;
}

/* outalternative = outitem { outitem } */
static OutExpr *outalternative(void)
{
    OutExpr *n, *t;

    n = outitem();
    while (LA!=TOK_SEMI && LA!=TOK_SLASH && LA!=TOK_LBRACKET && LA!=TOK_RPAREN) {
        t = new_outexpr(OE_SEQ);
        t->child[0] = n;
        t->child[1] = outitem();
        n = t;
    }
    return n;
}

/* outexpression = outalternative { "/" outalternative } */
OutExpr *outexpression(void)
{
    OutExpr *n, *t;

    n = outalternative();
    while (LA == TOK_SLASH) {
        match(TOK_SLASH);
        t = new_outexpr(OE_ALTER);
        t->child[0] = n;
        t->child[1] = outalternative();
        n = t;
    }
    return n;
}

/*
    item = "-"
         | ID "[" [ nodetest ] "]"
         | STR
         | nodename
         | label
         | basictype
*/
static NodeTestItem *item(void)
{
    NodeTestItem *n;

    n = calloc(1, sizeof(*n));
    switch (LA) {
    case TOK_MINUS:
        match(TOK_MINUS);
        n->kind = NT_ANY;
        break;
    case TOK_ID:
        n->kind = NT_NODE;
        n->node.name = strtab_new(token_string, strlen(token_string));
        match(TOK_ID);
        match(TOK_LBRACKET);
        n->node.test = (LA!=TOK_RBRACKET)?nodetest():NULL;
        match(TOK_RBRACKET);
        break;
    case TOK_STR:
        n->kind = NT_TEXT;
        n->text = strtab_new(token_string, strlen(token_string));
        match(TOK_STR);
        break;
    case TOK_STAR:
        n->kind = NT_NODENAME;
        n->nodnam = nodename();
        break;
    case TOK_HASH:
        n->kind = NT_LABEL;
        n->labslot = label();
        break;
    default: {
        SRNode *t;

        t = basictype();
        n->kind = NT_BASICTYPE;
        n->type = t->op;
        free(t);
    }
        break;
    }
    return n;
}

/* nodetest = item { "," item } */
NodeTestItem *nodetest(void)
{
    NodeTestItem *n, *t;

    t = n = item();
    while (LA == TOK_COMMA) {
        match(TOK_COMMA);
        t->next = item();
        t = t->next;
    }
    return n;
}

/* outrule = "[" [ nodetest ] "]" "=>" outexpression */
static OutRule *outrule(void)
{
    OutRule *r;

    r = calloc(1, sizeof(*r));
    match(TOK_LBRACKET);
    if (LA != TOK_RBRACKET)
        r->node_test = nodetest();
    match(TOK_RBRACKET);
    match(TOK_ARROW);
    r->out_expr = outexpression();
    return r;
}

/* outrulelist = outrule { outrule } */
static OutRule *outrulelist(void)
{
    OutRule *n, *t;

    t = n = outrule();
    while (LA == TOK_LBRACKET) {
        t->next = outrule();
        t = t->next;
    }
    return n;
}

/*
    coderule = ID outrulelist ";"
             | simplecoderule ";"
*/
static void coderule(void)
{
    skip_white();
    if (*curr == '[') {
        char rname[MAX_TOKSTR_LEN];

        if (LA == TOK_ID)
            strcpy(rname, token_string);
        match(TOK_ID);
        (void)lookup_code_rule(rname, 0, outrulelist());
    } else {
        simplecoderule();
    }
    match(TOK_SEMI);
}

/* ========================================================================== */
/* TREE BUILDING                                                              */
/* ========================================================================== */

/*
    ntest = ":" ID
          | "[" INT "]"
          | ":" ID "[" INT "]"
          | "+" STR
          | "*" "*"
          | "*"
*/
static SRNode *ntest(void)
{
    SRNode *n;

    switch (LA) {
    case TOK_COLON:
        match(TOK_COLON);
        if (LA == TOK_ID) {
            skip_white();
            n = new_srnode(*curr=='['?OP_NAMGENNODE:OP_NAMNODE);
            n->name = lookup_code_rule(token_string, 0, NULL)->name;
        }
        match(TOK_ID);
        if (LA == TOK_LBRACKET) {
            match(TOK_LBRACKET);
            if (LA == TOK_INT)
                n->nbr = atoi(token_string);
            match(TOK_INT);
            match(TOK_RBRACKET);
        }
        break;
    case TOK_LBRACKET:
        match(TOK_LBRACKET);
        if (LA == TOK_INT) {
            n = new_srnode(OP_GENNODE);
            n->nbr = atoi(token_string);
        }
        match(TOK_INT);
        match(TOK_RBRACKET);
        break;
    case TOK_PLUS:
        match(TOK_PLUS);
        if (LA == TOK_STR) {
            n = new_srnode(OP_PSTR);
            n->text = strtab_new(token_string, strlen(token_string));
        }
        match(TOK_STR);
        break;
    case TOK_STAR:
        match(TOK_STAR);
        if (LA == TOK_STAR) {
            match(TOK_STAR);
            n = new_srnode(OP_DUMPTREE);
        } else {
            n = new_srnode(OP_GENCODE);
        }
        break;
    default:
        assert(0);
    }
    return n;
}

/* ========================================================================== */
/* SYNTAX TESTS                                                               */
/* ========================================================================== */

static SRNode *alternativelist(void);

/*
    basictype = ".NUM"
              | ".ID"
              | ".OCT"
              | ".HEX"
              | ".SR"
              | ".CHR"
              | ".DIG"
              | ".LET"
*/
SRNode *basictype(void)
{
    SRNode *n;

    n = new_srnode(0);
    switch (LA) {
    case TOK_KW_NUM: n->op = OP_NUM; break;
    case TOK_KW_ID:  n->op = OP_ID; break;
    case TOK_KW_OCT: n->op = OP_OCT; break;
    case TOK_KW_HEX: n->op = OP_HEX; break;
    case TOK_KW_SR:  n->op = OP_SR; break;
    case TOK_KW_CHR: n->op = OP_CHR; break;
    case TOK_KW_DIG: n->op = OP_DIG; break;
    case TOK_KW_LET: n->op = OP_LET; break;
    default:
        match(-1);
    }
    match(LA);
    return n;
}

/*
    stest = STR
          | "." STR
          | ID
          | "@" INT
          | ".EMPTY"
          | "(" alternativelist ")"
          | "$" stest
          | basictype
*/
static SRNode *stest(void)
{
    SRNode *n;

    switch (LA) {
    case TOK_STR:
        n = new_srnode(OP_TEXT);
        n->text = strtab_new(token_string, strlen(token_string));
        match(TOK_STR);
        break;
    case TOK_DOT:
        match(TOK_DOT);
        if (LA == TOK_STR) {
            n = new_srnode(OP_PTEXT);
            n->text = strtab_new(token_string, strlen(token_string));
        }
        match(TOK_STR);
        break;
    case TOK_ID:
        n = new_srnode(OP_RULE);
        n->rule = lookup_syntax_rule(token_string, NULL);
        match(TOK_ID);
        break;
    case TOK_AT:
        match(TOK_AT);
        if (LA == TOK_INT) {
            n = new_srnode(OP_CHRCODE);
            n->chrcode = (unsigned char)atoi(token_string);
        }
        match(TOK_INT);
        break;
    case TOK_KW_EMPTY:
        n = new_srnode(OP_EMPTY);
        match(TOK_KW_EMPTY);
        break;
    case TOK_LPAREN:
        match(TOK_LPAREN);
        n = alternativelist();
        match(TOK_RPAREN);
        break;
    case TOK_DOLLAR:
        match(TOK_DOLLAR);
        n = new_srnode(OP_REP);
        n->child[0] = stest();
        break;
    default:
        n = basictype();
    }
    return n;
}

/* ========================================================================== */
/* SYNTAX RULES                                                               */
/* ========================================================================== */

/*
    qtest = ntest
          | stest [ "?" ( INT | STR ) "?" ]
*/
static SRNode *qtest(void)
{
    SRNode *n;

    if (LA==TOK_COLON || LA==TOK_LBRACKET || LA==TOK_PLUS || LA==TOK_STAR) {
        n = ntest();
    } else {
        n = stest();
        if (LA == TOK_QMARK) {
            match(TOK_QMARK);
            if (LA == TOK_INT) {
                n->err.type = ERR_TYPE_NUM;
                n->err.num = atoi(token_string);
                match(TOK_INT);
            } else if (LA == TOK_STR) {
                n->err.type = ERR_TYPE_STR;
                n->err.str = strtab_new(token_string, strlen(token_string));
                match(TOK_STR);
            } else {
                match(-1);
            }
            match(TOK_QMARK);
        }
    }
    return n;
}

/*
    test = ntest
         | stest
*/
static SRNode *test(void)
{
    if (LA==TOK_COLON || LA==TOK_LBRACKET || LA==TOK_PLUS || LA==TOK_STAR)
        return ntest();
    else
        return stest();
}

/* qtestlist = qtest { qtest } */
static SRNode *qtestlist(SRNode *n)
{
    SRNode *t;

    while (LA!=TOK_SLASH && LA!=TOK_SEMI && LA!=TOK_RPAREN) {
        t = new_srnode(OP_SEQ);
        t->child[0] = n;
        t->child[1] = qtest();
        n = t;
    }
    return n;
}

/* testlist = test { test } */
static SRNode *testlist(void)
{
    SRNode *n, *t;

    n = test();
    while (LA!=TOK_SLASH && LA!=TOK_SEMI && LA!=TOK_RPAREN) {
        t = new_srnode(OP_SEQ);
        t->child[0] = n;
        t->child[1] = test();
        n = t;
    }
    return n;
}

/*
    alternative = test [ qtestlist ]
                | "<-" testlist
*/
static SRNode *alternative(void)
{
    SRNode *n;

    if (LA == TOK_BACKARROW) {
        match(TOK_BACKARROW);
        n = new_srnode(OP_BT_ALTER);
        n->child[0] = testlist();
    } else {
        n = test();
        if (LA!=TOK_SLASH && LA!=TOK_SEMI && LA!=TOK_RPAREN)
            n = qtestlist(n);
    }
    return n;
}

/* alternativelist = alternative { "/" alternative } */
SRNode *alternativelist(void)
{
    SRNode *n, *t;

    n = alternative();
    while (LA == TOK_SLASH) {
        match(TOK_SLASH);
        t = new_srnode(OP_ALTER);
        t->child[0] = n;
        t->child[1] = alternative();
        n = t;
    }
    return n;
}

/* syntaxrule = ID "=" alternativelist ";" */
static void syntaxrule(void)
{
    char rname[MAX_TOKSTR_LEN];

    if (LA == TOK_ID)
        strcpy(rname, token_string);
    match(TOK_ID);
    match(TOK_EQ);
    (void)lookup_syntax_rule(rname, alternativelist());
    match(TOK_SEMI);
}

/* ========================================================================== */
/* TREE-META Program                                                          */
/* ========================================================================== */

/*
    rule = syntaxrule
         | coderule
         | symbolrule
*/
static void rule(void)
{
    skip_white();
    if (*curr == '=')
        syntaxrule();
    else if (*curr == ':')
        symbolrule();
    else
        coderule();
}

/* rulelist = rule { rule } */
static void rulelist(void)
{
    rule();
    while (LA!=TOK_KW_END && LA!=TOK_EOF)
        rule();
}

/*
    prefix = ".LIST"
           | ".DELIM" "(" INT "," INT "," INT ")"
*/
static void prefix(void)
{
    if (LA == TOK_KW_LIST) {
        match(TOK_KW_LIST);
    } else {
        match(TOK_KW_DELIM);
        match(TOK_LPAREN);
        match(TOK_INT);
        match(TOK_COMMA);
        match(TOK_INT);
        match(TOK_COMMA);
        match(TOK_INT);
        match(TOK_RPAREN);
    }
}

/* prefixlist = prefix { prefix } */
static void prefixlist(void)
{
    prefix();
    while (LA==TOK_KW_LIST || LA==TOK_KW_DELIM)
        prefix();
}

/* program = ".META" ID [ prefixlist ] rulelist ".END" */
static SyntaxRule *program(void)
{
    SyntaxRule *r;
    char *start;

    match(TOK_KW_META);
    if (LA == TOK_ID)
        start = strtab_new(token_string, strlen(token_string));
    match(TOK_ID);
    if (LA==TOK_KW_LIST || LA==TOK_KW_DELIM)
        prefixlist();
    rulelist();
    match(TOK_KW_END);
    for (r = syntax_rules; r != NULL; r = r->next)
        if (r->name == start)
            break;
    if (r==NULL || r->def==NULL) {
        eprintf("start symbol `%s' not defined", start);
        exit(EXIT_FAILURE);
    }
    return r;
}

SyntaxRule *read_input(char *filename)
{
    FILE *fp;
    char *buf;
    unsigned len;
    SyntaxRule *start;

    if ((fp=fopen(filename, "rb")) == NULL) {
        eprintf("cannot read file `%s'", filename);
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);
    buf = malloc(len+1);
    len = fread(buf, 1, len, fp);
    buf[len] = '\0';
    fclose(fp);

    input_path = filename;
    curr = buf;
    LA = get_token();
    start = program();
    free(buf);
    if (syntax_nundef!=0 || code_nundef!=0) {
        eprintf("the file `%s' contains the following undefined symbols:", filename);
        if (syntax_nundef != 0) {
            SyntaxRule *sr;

            fprintf(stderr, "Syntax rules:\n");
            for (sr = syntax_rules; sr != NULL; sr = sr->next)
                if (sr->def == NULL)
                    fprintf(stderr, "  %s\n", sr->name);
        }
        if (code_nundef != 0) {
            CodeRule *cr;

            fprintf(stderr, "Code rules:\n");
            for (cr = code_rules; cr != NULL; cr = cr->next)
                if (cr->def == NULL)
                    fprintf(stderr, "  %s\n", cr->name);
        }
        exit(EXIT_FAILURE);
    }
    TYPE_var = get_variable("TYPE");
    LEVEL_var = get_variable("LEVEL");
    VALUE_var = get_variable("VALUE");
    return start;
}
