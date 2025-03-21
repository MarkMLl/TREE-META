#ifndef TREEMETA_H_
#define TREEMETA_H_

#include "arithtype.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define MAX_TOKSTR_LEN 512

typedef enum {
    OP_TEXT,    /* STR */
    OP_NUM,     /* ".NUM" */
    OP_ID,      /* ".ID" */
    OP_OCT,     /* ".OCT" */
    OP_HEX,     /* ".HEX" */
    OP_SR,      /* ".SR" */
    OP_CHR,     /* ".CHR" */
    OP_DIG,     /* ".DIG" */
    OP_LET,     /* ".LET" */
    OP_LAB,
    OP_PTEXT,   /* "." STR (push string on match) */
    OP_CHRCODE, /* "@" INT */
    OP_EMPTY,   /* ".EMPTY" */
    OP_RULE,    /* ID */
    OP_ALTER,   /* "/" */
    OP_BT_ALTER,/* "<-" */
    OP_SEQ,     /* juxtaposition */
    OP_REP,     /* "$" */
    OP_PSTR,    /* "+" STR (push string without checking input) */
    OP_NAMNODE, /* ":" ID */
    OP_GENNODE, /* "[" INT "]" */
    OP_NAMGENNODE,/* ":" ID "[" INT "]" (OP_NAMNODE+OP_GENNODE) */
    OP_GENCODE, /* "*" */
    OP_DUMPTREE,/* "**" */
} OpKind;

enum {
    ERR_TYPE_NUM,   /* "?" INT "?" */
    ERR_TYPE_STR,   /* "?" STR "?" */
};

typedef struct SRNode SRNode;
typedef struct SyntaxRule SyntaxRule;

struct SRNode {
    OpKind op;
    union {
        int chrcode;
        char *text;
        SyntaxRule *rule;
        SRNode *child[2];
        struct {
            char *name;
            int nbr;
        };
    };
    struct {
        int type;
        union {
            int num;
            char *str;
        };
    } err;
};

struct SyntaxRule {
    char *name;
    SRNode *def;
    SyntaxRule *next;
};

/*
    Node type of trees generated by the syntax rules.
*/
typedef struct TreeNode TreeNode;
struct TreeNode {
    int isleaf;
    union {
        struct {
            char *name;
            int nbr;
            TreeNode *child;
        } node;
        struct {
            OpKind type;
            union {
                char *text;
                int labslot;
            };
        } str;
    };
    TreeNode *sibling;
};

typedef struct CodeRule CodeRule;
typedef struct OutRule OutRule;
typedef struct NodeName NodeName;
typedef struct NodeTestItem NodeTestItem;
typedef struct OutExpr OutExpr;
typedef struct ArithExpr ArithExpr;

typedef enum {
    A_VAR, A_INT, A_NEG, A_ADD,
    A_SUB, A_LSH, A_URSH, A_SRSH,
    A_AND, A_EOR, A_OR, A_FUNC,
    A_ASN, A_COMPL, A_LNEG,

    /* maintain order of what follows */
    A_MUL, A_UDIV, A_SDIV, A_UMOD,
    A_SMOD, A_EQ, A_NEQ, A_UGT,
    A_ULT, A_UGET, A_ULET, A_SGT,
    A_SLT, A_SGET, A_SLET,
} ArithKind;

typedef enum {
    /* Functions */
    F_LEN,      /* LEN[nodename] */
    F_CODE,     /* CODE[nodename] */
    F_CONV,     /* CONV[nodename] */
    F_XCONV,    /* XCONV[nodename] */
    F_POP,      /* POP[0] */
    F_LOOK,     /* LOOK[nodename] */
    F_CLEAR,    /* CLEAR[expression] */
    /* Subroutines/Procedures */
    F_PUSH,     /* PUSH[expression] */
    F_OUT,      /* OUT[expression] */
    F_OUTS,     /* OUTS[expression] */
    F_OUTL,     /* OUTL[nodename] */
    F_OUTC,     /* OUTC[nodename] */
    F_ENTER,    /* ENTER[nodename] */
} Function;

struct ArithExpr {
    ArithKind kind;
    union {
        struct {
            Function func;
            union {
                ArithExpr *expr;
                NodeName *nodnam;
            } arg;
        };
        struct {
            ArithType *lhs;
            ArithExpr *rhs;
        };
        ArithExpr *child[2];
        ArithType *var;
        ArithType val;
    };
    ArithExpr *next;
};

struct NodeName {
    int branch;
    NodeName *next;
};

typedef enum {
    NT_ANY,
    NT_NODE,
    NT_TEXT,
    NT_NODENAME,
    NT_LABEL,
    NT_BASICTYPE,
} TestKind;

struct NodeTestItem {
    TestKind kind;
    union {
        struct {
            char *name;
            NodeTestItem *test;
        } node;
        char *text;
        NodeName *nodnam;
        OpKind type;
        int labslot;
    };
    NodeTestItem *next;
};

typedef enum {
    OE_ALTER,
    OE_SEQ,
    OE_NEWLINE,
    OE_TEXT,
    OE_CHRCODE,
    OE_NODENAME,
    OE_CRCALL,
    OE_ARITH,
    OE_EMPTY,
    OE_LABEL,
    // ! STR
} OutExprKind;

struct OutExpr {
    OutExprKind kind;
    union {
        OutExpr *child[2];
        char *text;
        int chrcode;
        NodeName *nodnam;
        struct {
            TreeNode *tree;
            char nodnam_arg;
            char lab_arg;
        } cr;
        ArithExpr *expr;
        int labslot;
    };
};

struct OutRule {
    NodeTestItem *node_test;
    OutExpr *out_expr;
    OutRule *next;
};

#define CR_ATTR_SIMPLE 1
#define CR_ATTR_SYMBOL 2

struct CodeRule {
    char *name;
    unsigned attr;
    OutRule *def;
    CodeRule *next;
};

extern SyntaxRule *syntax_rules;
extern CodeRule *code_rules;
extern ArithType *TYPE_var, *LEVEL_var, *VALUE_var;
extern int MFLAG;
extern char *prog_name;

#define eprintf(...)                        \
    do {                                    \
        fprintf(stderr, "%s: ", prog_name); \
        fprintf(stderr, __VA_ARGS__);       \
        fprintf(stderr, "\n");              \
    } while (0)

#endif
