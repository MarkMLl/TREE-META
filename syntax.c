#include "syntax.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <assert.h>
#include "code.h"
#include "strtab.h"
#include "stack.h"

typedef struct State State;

#define BUF_STK_STEP 8
#define BUF_NNODE    32

static char *inbuf, *inpos;
static char *input_path;
static int line_counter = 1;

static Stack *tree_stack;
static Stack *call_stack;

static void dump_backtrace(void)
{
    int i, n;

    fprintf(stderr, "\nSyntax rules backtrace:\n");
    for (i=0, n=stack_length(call_stack); i < n; i++)
        fprintf(stderr, "%*s%s%s\n", i*2, "", (char *)stack_index(call_stack, i),
        (i==n-1)?" <-":"");
}

#define HALT(...)                               \
    do {                                        \
        fprintf(stderr, "%s: ", prog_name);     \
        fprintf(stderr, __VA_ARGS__);           \
        fprintf(stderr, "\n");                  \
        dump_backtrace();                       \
        exit(EXIT_FAILURE);                     \
    } while (0)

static struct {
    TreeNode *buf;
    int nused;
} *buf_stack;
static int buf_stack_max, buf_stack_top;

struct State {
    char *ip;
    int lc;
    Stack *ts;
    int bs_top, bs_nused;
};

static void save_state(State *st)
{
    st->ip = inpos;
    st->lc = line_counter;
    st->bs_top = buf_stack_top;
    st->bs_nused = buf_stack[buf_stack_top].nused;
    st->ts = stack_dup(tree_stack);
}

static void restore_state(State *st)
{
    inpos = st->ip;
    line_counter = st->lc;
    buf_stack_top = st->bs_top;
    buf_stack[buf_stack_top].nused = st->bs_nused;
    stack_cpy(tree_stack, st->ts);
    stack_destroy(st->ts);
}

static TreeNode *new_tree_node(int isleaf)
{
    TreeNode *n;

    if (buf_stack[buf_stack_top].nused >= BUF_NNODE) {
        if (++buf_stack_top >= buf_stack_max) {
            buf_stack_max += BUF_STK_STEP;
            buf_stack = realloc(buf_stack, buf_stack_max*sizeof(buf_stack[0]));
            memset(buf_stack+buf_stack_top, 0, sizeof(buf_stack[0])*(buf_stack_max-buf_stack_top));
        }
        if (buf_stack[buf_stack_top].buf == NULL)
            buf_stack[buf_stack_top].buf = malloc(BUF_NNODE*sizeof(buf_stack[0].buf[0]));
        buf_stack[buf_stack_top].nused = 0;
    }
    n = buf_stack[buf_stack_top].buf+buf_stack[buf_stack_top].nused++;
    if (!(n->isleaf=isleaf))
        n->node.child = NULL;
    n->sibling = NULL;
    return n;
}

static void push_node(TreeNode *n)
{
    stack_push(tree_stack, n);
}

static TreeNode *pop_node(void)
{
    if (stack_isempty(tree_stack))
        HALT("tree stack underflow");
    return stack_pop(tree_stack);
}

static void push_leaf(OpKind type, char *text)
{
    TreeNode *n;

    n = new_tree_node(TRUE);
    n->str.type = type;
    n->str.text = text;
    stack_push(tree_stack, n);
}

static void dump_tree2(TreeNode *tnode)
{
    static char *opstr[] = {
        "String", ".NUM", ".ID",
        ".OCT", ".HEX", ".SR",
        ".CHR", ".DIG", ".LET",
    };
    static int vertex_counter;

    if (tnode->isleaf) {
        assert(tnode->str.type <= OP_LET);
        printf("V%d[label=\"%s\\n(`%s')\"]\n", vertex_counter++, opstr[tnode->str.type], tnode->str.text);
    } else {
        int v;
        TreeNode *p;

        v = vertex_counter++;
        printf("V%d[label=\"%s\"]\n", v, tnode->node.name);
        for (p = tnode->node.child; p != NULL; p = p->sibling) {
            printf("V%d -> V%d\n", v, vertex_counter);
            dump_tree2(p);
        }
    }
}

static void dump_tree(TreeNode *tnode)
{
    printf("digraph {\n");
    dump_tree2(tnode);
    printf("}\n");
}

static void report_syntax_error(int num, char *str)
{
    char *beg, *end;

    fprintf(stderr, "%s: %s:%d: syntax error `", prog_name, input_path, line_counter);
    if (str == NULL)
        fprintf(stderr, "%d", num);
    else
        fprintf(stderr, "%s", str);
    fprintf(stderr, "' in\n");

    for (beg = inpos; beg>inbuf && *beg!='\n'; beg--)
        ;
    if (*beg == '\n')
        ++beg;
    for (end = inpos; *end!='\0' && *end!='\n'; end++)
        ;
    fprintf(stderr, "%.*s\n", end-beg, beg);
    fprintf(stderr, "%*s^\n", inpos-beg, "");

    dump_backtrace();
    exit(EXIT_FAILURE);
}

static void skip_white(void)
{
    int done;

    done = FALSE;
    while (!done) {
        done = TRUE;
        if (isspace(*inpos)) {
            do {
                if (*inpos == '\n')
                    ++line_counter;
                ++inpos;
            } while (isspace(*inpos));
            done = FALSE;
        }
        if (inpos[0]=='/' && inpos[1]=='*') {
            int sl;

            sl = line_counter;
            inpos += 2;
            while (inpos[0]!='\0' && (inpos[0]!='*' || inpos[1]!='/'))
                if (*inpos++ == '\n')
                    ++line_counter;
            if (*inpos == '\0') {
                fprintf(stderr, "%s: %s:%d: unterminated comment (starts at line %d)\n",
                prog_name, input_path, line_counter, sl);
                exit(EXIT_FAILURE);
            }
            inpos += 2;
            done = FALSE;
        }
    }
}

static void recognize(SRNode *n, int bt)
{
    int i;
    TreeNode *tnode;
    static char *node_name;
    char *s, *t, *q;
    char tokbuf[MAX_TOKSTR_LEN];
    static int synerr;

    switch (n->op) {
    case OP_ALTER:  /* alternation ("/") */
        recognize(n->child[0], bt);
        if (!MFLAG && !synerr)
            recognize(n->child[1], bt);
        break;
    case OP_BT_ALTER: { /* backtracking alternative ("<-") */
        State st;

        save_state(&st);
        recognize(n->child[0], TRUE);
        if (synerr) {
            restore_state(&st);
            synerr = FALSE;
        } else {
            stack_destroy(st.ts);
        }
    }
        break;
    case OP_SEQ:    /* sequence (juxtaposition) */
        recognize(n->child[0], bt);
        if (!MFLAG)
            break;
        recognize(n->child[1], bt);
        if (!MFLAG) {
            if (bt) {
                synerr = TRUE;
                break;
            }
            report_syntax_error(n->child[1]->err.num,
            (n->child[1]->err.type==ERR_TYPE_STR)?n->child[1]->err.str:NULL);
        }
        break;
    case OP_REP:    /* repetition ("$") */
        do
            recognize(n->child[0], bt);
        while (MFLAG);
        MFLAG = synerr?FALSE:TRUE;
        break;

    case OP_EMPTY:  /* ".EMPTY" */
        MFLAG = TRUE;
        break;
    case OP_RULE:   /* ID */
        stack_push(call_stack, n->rule->name);
        recognize(n->rule->def, bt);
        stack_pop(call_stack);
        break;
    case OP_TEXT:   /* STR */
    case OP_PTEXT:  /* "." STR */
        skip_white();
        for (s=inpos, t=n->text; *t!='\0' && *s==*t; s++, t++)
            ;
        if (*t == '\0') {
            inpos = s;
            if (n->op == OP_PTEXT)
                push_leaf(OP_TEXT, n->text);
            MFLAG = TRUE;
        } else {
            MFLAG = FALSE;
        }
        break;
    case OP_NUM:    /* ".NUM" */
        skip_white();
        if (isdigit(*inpos)) {
            q = tokbuf;
            do
                *q++ = *inpos++;
            while (isdigit(*inpos));
            *q = '\0';
            push_leaf(OP_NUM, strtab_new(tokbuf, q-tokbuf));
            MFLAG = TRUE;
        } else {
            MFLAG = FALSE;
        }
        break;
    case OP_ID:     /* ".ID" */
        skip_white();
        if (isalpha(*inpos)) {
            q = tokbuf;
            do
                *q++ = *inpos++;
            while (isalnum(*inpos));
            *q = '\0';
            push_leaf(OP_ID, strtab_new(tokbuf, q-tokbuf));
            MFLAG = TRUE;
        } else {
            MFLAG = FALSE;
        }
        break;
    case OP_OCT:    /* ".OCT" */
        skip_white();
        if (*inpos>='0' && *inpos<='7') {
            q = tokbuf;
            do
                *q++ = *inpos++;
            while (*inpos>='0' && *inpos<='7');
            *q = '\0';
            push_leaf(OP_OCT, strtab_new(tokbuf, q-tokbuf));
            MFLAG = TRUE;
        } else {
            MFLAG = FALSE;
        }
        break;
    case OP_HEX:    /* ".HEX" */
        skip_white();
        if (isxdigit(*inpos)) {
            q = tokbuf;
            do
                *q++ = *inpos++;
            while (isxdigit(*inpos));
            *q = '\0';
            push_leaf(OP_HEX, strtab_new(tokbuf, q-tokbuf));
            MFLAG = TRUE;
        } else {
            MFLAG = FALSE;
        }
        break;
    case OP_SR:     /* ".SR" */
        skip_white();
        if (*(s=inpos) == '\'') {
            ++s;
            q = tokbuf;
            while (*s!='\'' && *s!='\0' && *s!='\n')
                *q++ = *s++;
        }
        if (*s == '\'') {
            *q = '\0';
            inpos = ++s;
            push_leaf(OP_SR, strtab_new(tokbuf, q-tokbuf));
            MFLAG = TRUE;
        } else {
            MFLAG = FALSE;
        }
        break;
    case OP_CHR:    /* ".CHR" */
        tokbuf[0] = *inpos++;
        tokbuf[1] = '\0';
        push_leaf(OP_CHR, strtab_new(tokbuf, 1));
        MFLAG = TRUE;
        break;
    case OP_DIG:    /* ".DIG" */
        skip_white();
        if (isdigit(*inpos)) {
            tokbuf[0] = *inpos++;
            tokbuf[1] = '\0';
            push_leaf(OP_DIG, strtab_new(tokbuf, 1));
            MFLAG = TRUE;
        } else {
            MFLAG = FALSE;
        }
        break;
    case OP_LET:    /* ".LET" */
        skip_white();
        if (isalpha(*inpos)) {
            tokbuf[0] = *inpos++;
            tokbuf[1] = '\0';
            push_leaf(OP_LET, strtab_new(tokbuf, 1));
            MFLAG = TRUE;
        } else {
            MFLAG = FALSE;
        }
        break;
    case OP_CHRCODE: /* "@" INT */
        skip_white();
        if (*inpos == n->chrcode) {
            ++inpos;
            MFLAG = TRUE;
        } else {
            MFLAG = FALSE;
        }
        break;

    case OP_PSTR:       /* "+" STR */
        push_leaf(OP_TEXT, n->text);
        break;
    case OP_NAMNODE:    /* ":" ID */
        node_name = n->name;
        break;
    case OP_GENNODE:    /* "[" INT "]" */
        if (node_name == NULL)
            HALT("node-building operation [%d] is not preceded by a name definition", n->nbr);
    case OP_NAMGENNODE: /* ":" ID "[" INT "]" */
        tnode = new_tree_node(FALSE);
        tnode->node.name = (n->op==OP_GENNODE)?node_name:n->name;
        if ((tnode->node.nbr=n->nbr) > 0) {
            TreeNode *p1, *p2;

            p1 = p2 = pop_node();
            for (i = 1; i < n->nbr; i++) {
                p1 = pop_node();
                p1->sibling = p2;
                p2 = p1;
            }
            tnode->node.child = p1;
        }
        push_node(tnode);
        node_name = NULL;
        break;
    case OP_GENCODE:    /* "*" */
    case OP_DUMPTREE:   /* "**" */
        if (!stack_isempty(tree_stack)) {
            if (n->op == OP_GENCODE) {
                gencode(stack_pop(tree_stack));
                if (!MFLAG)
                    HALT("code generation initiated by * failed");
            } else {
                dump_tree(stack_pop(tree_stack));
                MFLAG = TRUE;
            }
            stack_trim(tree_stack, 0);
            buf_stack_top = 0;
            buf_stack[buf_stack_top].nused = 0;
        }
        break;
    default:
        assert(0);
    }
}

void parse(SyntaxRule *start, char *filename)
{
    FILE *fp;
    unsigned len;

    if ((fp=fopen(filename, "rb")) == NULL) {
        eprintf("cannot read file `%s'", filename);
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);
    inbuf = malloc(len+1);
    len = fread(inbuf, 1, len, fp);
    inbuf[len] = '\0';
    fclose(fp);

    input_path = filename;
    inpos = inbuf;
    tree_stack = stack_new();
    buf_stack_max = BUF_STK_STEP;
    buf_stack = calloc(buf_stack_max, sizeof(*buf_stack));
    buf_stack[buf_stack_top].buf = malloc(BUF_NNODE*sizeof(buf_stack[0].buf[0]));
    call_stack = stack_new();
    stack_push(call_stack, start->name);
    recognize(start->def, 0);

    if (!stack_isempty(tree_stack))
        dump_tree(stack_peek(tree_stack));
}
