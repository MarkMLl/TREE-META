#include "code.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "stack.h"
#include "symtab.h"

static Stack *call_stack;
static int label_counter = 1;
static int match_tree(NodeTestItem *ntest, TreeNode *tnode, TreeNode *parent);
static void do_coderule(CodeRule *rule, TreeNode *tnode, int *labs);

static TreeNode *get_br(TreeNode *tnode, int n)
{
    for (tnode = tnode->node.child; n > 0; n--)
        tnode = tnode->sibling;
    return tnode;
}

static void dump_backtrace(void)
{
    int i, n;

    fprintf(stderr, "\nCode rules backtrace:\n");
    for (i=0, n=stack_length(call_stack); i < n; i++)
        fprintf(stderr, "%*s%s%s\n", i*2, "", (char *)stack_index(call_stack, i),
        (i==n-1)?" <-":"");
}

static char *strnodnam(NodeName *nn)
{
    char *p;
    static char buf[128];

    for (p = buf; nn != NULL; nn = nn->next)
        p += sprintf(p, "*%d%s", nn->branch, nn->next?":":"");
    *p = '\0';
    return buf;
}

#define HALT(...)                               \
    do {                                        \
        fprintf(stderr, "%s: ", prog_name);     \
        fprintf(stderr, __VA_ARGS__);           \
        fprintf(stderr, "\n");                  \
        dump_backtrace();                       \
        exit(EXIT_FAILURE);                     \
    } while (0)

static CodeRule *get_coderule(char *name)
{
    CodeRule *r;

    for (r = code_rules; r != NULL; r = r->next)
        if (r->name == name)
            return r;
    assert(0);
}

static TreeNode *decode_nodename2(TreeNode *tnode, NodeName *nodnam)
{
    if (nodnam == NULL)
        return tnode;
    if (tnode->isleaf || nodnam->branch>tnode->node.nbr)
        return NULL;
    return decode_nodename2(get_br(tnode, nodnam->branch-1), nodnam->next);
}

static TreeNode *decode_nodename(TreeNode *tnode, NodeName *nodnam, int errlab)
{
    if ((tnode=decode_nodename2(tnode, nodnam)) == NULL)
        HALT("attempt to access a non-existent branch by %s", strnodnam(nodnam));
    else if (errlab && tnode->isleaf && tnode->str.type==OP_LAB)
        HALT("using nodename %s to access label argument", strnodnam(nodnam));
    return tnode;
}

static TreeNode *free_list;

static TreeNode *new_tree_node(int isleaf)
{
    TreeNode *n;

    if ((n=free_list) == NULL)
        n = malloc(sizeof(*n));
    else
        free_list = n->sibling;
    n->isleaf = isleaf;
    n->sibling = NULL;
    return n;
}

static void free_tree(TreeNode *tnode)
{
    TreeNode *p;

    for (p = tnode; p->sibling != NULL; p = p->sibling)
        ;
    p->sibling = free_list;
    free_list = tnode;
}

#define MAX_SYSSTACK 128
static ArithType sysstack[MAX_SYSSTACK];
static int sysstack_top = -1;

static void badarg(Function func, NodeName *nodnam)
{
    static char *funcstr[] = {
        "LEN", "CODE", "CONV",
        "XCONV", "POP", "LOOK",
        "CLEAR", "PUSH", "OUT",
        "OUTS", "OUTL", "OUTC",
        "ENTER",
    };
    HALT("argument %s to function %s has the wrong type", strnodnam(nodnam), funcstr[func]);
}

static ArithType eval_expr(ArithExpr *e, TreeNode *tnode)
{
    switch (e->kind) {
    case A_VAR: return *e->var;
    case A_INT: return e->val;
    case A_NEG: return -eval_expr(e->child[0], tnode);
    case A_LNEG: return !eval_expr(e->child[0], tnode);
    case A_COMPL: return ~eval_expr(e->child[0], tnode);
    case A_MUL: return eval_expr(e->child[0], tnode)*eval_expr(e->child[1], tnode);
    case A_UDIV: return eval_expr(e->child[0], tnode)/eval_expr(e->child[1], tnode);
    case A_SDIV: return (SignedArithType)eval_expr(e->child[0], tnode)/(SignedArithType)eval_expr(e->child[1], tnode);
    case A_UMOD: return eval_expr(e->child[0], tnode)%eval_expr(e->child[1], tnode);
    case A_SMOD: return (SignedArithType)eval_expr(e->child[0], tnode)%(SignedArithType)eval_expr(e->child[1], tnode);
    case A_ADD: return eval_expr(e->child[0], tnode)+eval_expr(e->child[1], tnode);
    case A_SUB: return eval_expr(e->child[0], tnode)-eval_expr(e->child[1], tnode);
    case A_LSH: return eval_expr(e->child[0], tnode)<<eval_expr(e->child[1], tnode);
    case A_URSH: return eval_expr(e->child[0], tnode)>>eval_expr(e->child[1], tnode);
    case A_SRSH: return (SignedArithType)eval_expr(e->child[0], tnode)>>eval_expr(e->child[1], tnode);
    case A_AND: return eval_expr(e->child[0], tnode)&eval_expr(e->child[1], tnode);
    case A_EOR: return eval_expr(e->child[0], tnode)^eval_expr(e->child[1], tnode);
    case A_OR: return eval_expr(e->child[0], tnode)|eval_expr(e->child[1], tnode);
    case A_FUNC:
        switch (e->func) {
        case F_LEN:     /* LEN[nodename] */
        case F_CODE:    /* CODE[nodename] */
        case F_CONV:    /* CONV[nodename] */
        case F_XCONV:   /* XCONV[nodename] */
        case F_OUTL:    /* OUTL[nodename] */
        case F_OUTC:    /* OUTC[nodename] */
        case F_ENTER:   /* ENTER[nodename] */
        case F_LOOK:    /* LOOK[nodename] */
            if (!(tnode=decode_nodename(tnode, e->arg.nodnam, TRUE))->isleaf)
                badarg(e->func, e->arg.nodnam);
            switch (e->func) {
            case F_LEN:
                return strlen(tnode->str.text);
            case F_CODE:
                return (unsigned char)*tnode->str.text;
            case F_CONV:
                if (tnode->str.type != OP_NUM)
                    badarg(e->func, e->arg.nodnam);
                return (ArithType)strtoul(tnode->str.text, NULL, 10);
            case F_XCONV:
                if (tnode->str.type != OP_HEX)
                    badarg(e->func, e->arg.nodnam);
                return (ArithType)strtoul(tnode->str.text, NULL, 16);
            case F_OUTL:
                printf("%u", strlen(tnode->str.text));
                break;
            case F_OUTC:
                printf("%c", *tnode->str.text);
                break;
            case F_ENTER:
                symtab_enter(tnode->str.text, *LEVEL_var, *TYPE_var, *VALUE_var);
                break;
            case F_LOOK:
                return symtab_look(tnode->str.text, LEVEL_var, TYPE_var, VALUE_var);
            }
            break;
        case F_POP:     /* POP[0] */
            if (sysstack_top < 0)
                HALT("system stack underflow");
            return sysstack[sysstack_top--];
        case F_PUSH: {  /* PUSH[expression] */
            ArithType val;

            val = eval_expr(e->arg.expr, tnode);
            if (++sysstack_top >= MAX_SYSSTACK)
                HALT("system stack overflow");
            sysstack[sysstack_top] = val;
        }
            break;
        case F_OUT:     /* OUT[expression] */
            printf("%u", eval_expr(e->arg.expr, tnode));
            break;
        case F_OUTS:    /* OUTS[expression] */
            printf("%d", eval_expr(e->arg.expr, tnode));
            break;
        case F_CLEAR:   /* CLEAR[expression] */
            return symtab_clear(eval_expr(e->arg.expr, tnode));
        default:
            assert(0);
        }
        break;
    case A_ASN:
        *e->lhs = eval_expr(e->rhs, tnode);
        break;
    case A_EQ: return *e->lhs==eval_expr(e->rhs, tnode);
    case A_NEQ: return *e->lhs!=eval_expr(e->rhs, tnode);
    case A_UGT: return *e->lhs>eval_expr(e->rhs, tnode);
    case A_SGT: return (SignedArithType)*e->lhs>(SignedArithType)eval_expr(e->rhs, tnode);
    case A_ULT: return *e->lhs<eval_expr(e->rhs, tnode);
    case A_SLT: return (SignedArithType)*e->lhs<(SignedArithType)eval_expr(e->rhs, tnode);
    case A_UGET: return *e->lhs>=eval_expr(e->rhs, tnode);
    case A_SGET: return (SignedArithType)*e->lhs>=(SignedArithType)eval_expr(e->rhs, tnode);
    case A_ULET: return *e->lhs<=eval_expr(e->rhs, tnode);
    case A_SLET: return (SignedArithType)*e->lhs<=(SignedArithType)eval_expr(e->rhs, tnode);
    default:
        assert(0);
    }
    return 0; /* dummy value */
}

static void do_outexpression(OutExpr *expr, TreeNode *tnode, int *labs)
{
    TreeNode *t, q;

    switch (expr->kind) {
    case OE_ALTER:
        do_outexpression(expr->child[0], tnode, labs);
        if (!MFLAG)
            do_outexpression(expr->child[1], tnode, labs);
        break;
    case OE_SEQ:
        do_outexpression(expr->child[0], tnode, labs);
        if (!MFLAG)
            break;
        do_outexpression(expr->child[1], tnode, labs);
        if (!MFLAG)
            HALT("non-first outitem failed");
        break;
    case OE_NEWLINE:
        printf("\n");
        MFLAG = TRUE;
        break;
    case OE_TEXT:
        printf("%s", expr->text);
        MFLAG = TRUE;
        break;
    case OE_CHRCODE:
        printf("%c", expr->chrcode);
        MFLAG = TRUE;
        break;
    case OE_NODENAME:
        gencode(decode_nodename(tnode, expr->nodnam, TRUE));
        break;
    case OE_CRCALL:
        t = expr->cr.tree;
        q.node.child = NULL;
        if (expr->cr.nodnam_arg) {
            TreeNode h, *x, *y;

            q.isleaf = FALSE;
            q.node.name = t->node.name;
            q.node.nbr = t->node.nbr;
            for (x=&h, y=t->node.child; y != NULL; y = y->sibling) {
                x->sibling = new_tree_node(y->isleaf);
                x = x->sibling;
                if (y->isleaf)
                    *x = *y;
                else
                    *x = *decode_nodename(tnode, (NodeName *)y->node.name, TRUE);
                x->sibling = NULL;
            }
            q.node.child = h.sibling;
            t = &q;
        }
        if (expr->cr.lab_arg) {
            /* generate any passed and not-yet-generated label */
            TreeNode *a;

            for (a = t->node.child; a != NULL; a = a->sibling)
                if (a->isleaf && a->str.type==OP_LAB && !labs[a->str.labslot])
                    labs[a->str.labslot] = label_counter++;
        }
        do_coderule(get_coderule(t->node.name), t, expr->cr.lab_arg?labs:NULL);
        if (q.node.child != NULL)
            free_tree(q.node.child);
        break;
    case OE_ARITH: {
        ArithExpr *e;
        ArithType res;

        for (e = expr->expr; ; e = e->next) {
            res = eval_expr(e, tnode);
            if (e->next == NULL)
                break;
        }
        if (e->kind>=A_EQ && e->kind<=A_SLET
        || e->kind==A_FUNC && e->func==F_LOOK)
            MFLAG = res;
        else
            MFLAG = TRUE;
    }
        break;
    case OE_EMPTY:
        MFLAG = TRUE;
        break;
    case OE_LABEL:
        if (labs == NULL)
            HALT("use of labels in the wrong place");
        if (!labs[expr->labslot])
            labs[expr->labslot] = label_counter++;
        printf("L%d", labs[expr->labslot]);
        MFLAG = TRUE;
        break;
    default:
        assert(0);
    }
}

static int match_branch(NodeTestItem *btest, TreeNode *branch, TreeNode *parent)
{
    TreeNode *t;

    switch (btest->kind) {
    case NT_ANY:
        return TRUE;
    case NT_NODE:
        return (!branch->isleaf
        && branch->node.name==btest->node.name
        && match_tree(btest->node.test, branch, parent));
    case NT_TEXT:
        return (branch->isleaf
        && branch->str.type!=OP_LAB
        && branch->str.text==btest->text);
    case NT_NODENAME:
        return (branch->isleaf && branch->str.type!=OP_LAB
        && (t=decode_nodename(parent, btest->nodnam, FALSE))->isleaf && t->str.type!=OP_LAB
        && branch->str.text==t->str.text);
    case NT_LABEL:
        return (branch->isleaf && branch->str.type==OP_LAB);
    case NT_BASICTYPE:
        if (!branch->isleaf)
            return FALSE;
        switch (btest->type) {
        case OP_NUM:    return branch->str.type == OP_NUM;
        case OP_ID:     return branch->str.type == OP_ID;
        case OP_OCT:    return branch->str.type == OP_OCT;
        case OP_HEX:    return branch->str.type == OP_HEX;
        case OP_SR:     return branch->str.type == OP_SR;
        case OP_CHR:    return branch->str.type==OP_CHR
                        || branch->str.type==OP_DIG
                        || branch->str.type==OP_LET;
        case OP_DIG:    return branch->str.type==OP_DIG
                        || branch->str.type==OP_CHR && isdigit(*branch->str.text);
        case OP_LET:    return branch->str.type==OP_LET
                        || branch->str.type==OP_CHR && isalpha(*branch->str.text);
        default:
            return FALSE;
        }
    }
    assert(0);
}

int match_tree(NodeTestItem *ntest, TreeNode *tnode, TreeNode *parent)
{
    int i;
    NodeTestItem *t;

    assert(!tnode->isleaf);
    i = 0;
    for (t = ntest; t != NULL; t = t->next)
        ++i;
    if (tnode->node.nbr != i)
        return FALSE;
    i = 0;
    for (t = ntest; t != NULL; t=t->next, i++)
        if (!match_branch(t, get_br(tnode, i), parent))
            return FALSE;
    return TRUE;
}

void do_coderule(CodeRule *rule, TreeNode *tnode, int *labs)
{
    if (call_stack == NULL)
        call_stack = stack_new();
    stack_push(call_stack, rule->name);
    if (rule->attr & CR_ATTR_SIMPLE) {
        do_outexpression(rule->def->out_expr, tnode, NULL);
    } else if (rule->attr & CR_ATTR_SYMBOL) {
        TreeNode x, y;

        /*
            Fake a tree of the form

                <symbol-rule-name>
                        |
                   <symbol-name>

            We adjust <symbol-name> for each entry in the table.
        */
        x.isleaf = FALSE;
        x.node.name = tnode->node.name;
        x.node.nbr = 1;
        x.node.child = &y;
        y.isleaf = TRUE;
        y.str.type = OP_TEXT;
        y.sibling = NULL;
        y.str.text = symtab_iterate(TRUE, LEVEL_var, TYPE_var, VALUE_var);
        while (y.str.text != NULL) {
            (void)do_outexpression(rule->def->out_expr, &x, NULL);
            y.str.text = symtab_iterate(FALSE, LEVEL_var, TYPE_var, VALUE_var);
        }
    } else {
        OutRule *n;

        for (n = rule->def; n != NULL; n = n->next) {
            if (match_tree(n->node_test, tnode, tnode)) {
                int newlabs[4] = { 0 };

                if (labs != NULL) {
                    int i;
                    NodeTestItem *t;

                    /*
                        At this point we know that the caller (tree) is passing
                        labels and that the callee (nodetest) is expecting labels.
                    */
                    for (i=0, t=n->node_test; t != NULL; i++, t=t->next)
                        if (t->kind == NT_LABEL)
                            newlabs[t->labslot] = labs[get_br(tnode, i)->str.labslot];
                }
                do_outexpression(n->out_expr, tnode, newlabs);
                goto done;
            }
        }
        /* no outrule for a node of this form */
        MFLAG = FALSE;
    }
done:
    stack_pop(call_stack);
}

void gencode(TreeNode *tnode)
{
    if (tnode->isleaf) {
        printf("%s", tnode->str.text);
        MFLAG = TRUE;
    } else {
        do_coderule(get_coderule(tnode->node.name), tnode, NULL);
    }
}
