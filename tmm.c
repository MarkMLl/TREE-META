#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include "strtab.h"
#include "symtab.h"
#include "stack.h"

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

/* ========================================================================== */
/* ASSEMBLING                                                                 */
/* ========================================================================== */

#define LINEBUFSIZ  1024
#define LABTABSIZ   511

typedef struct LabSym LabSym;
typedef struct IRec IRec;
typedef struct IDescr IDescr;
typedef struct FixUp FixUp;

typedef enum {
    I_ERCHK,
    I_ERSTR,
    I_OER,
    I_CGER,
    I_CALL,
    I_R,
    I_RC,
    I_BF,
    I_BT,
    I_SET,
    I_BR,
    I_SBTP,
    I_CBTP,
    I_TST,
    I_SRP,
    I_ID,
    I_NUM,
    I_OCT,
    I_HEX,
    I_SR,
    I_CHR,
    I_DIG,
    I_LET,
    I_CHRCODE,
    I_NDLBL,
    I_NDMK,
    I_SRARG,
    I_OUTRE,
    I_DUMP,
    I_BRARG,
    I_LBARG,
    I_CNTCK,
    I_NXTBR,
    I_RSTBR,
    I_CKNDNAM,
    I_SAVEP,
    I_CHASE,
    I_RSTRP,
    I_CKBR,
    I_MOVTO,
    I_MOVTOBASE,
    I_OUTCL,
    I_CKTXT,
    I_CKNUM,
    I_CKID,
    I_CKOCT,
    I_CKHEX,
    I_CKSR,
    I_CKCHR,
    I_CKDIG,
    I_CKLET,
    I_CKLAB,
    I_OUTSR,
    I_OUTCH,
    I_ONL,
    I_GNLB,
    I_CLABS,
    I_LOAD,
    I_LOADI,
    I_STORE,
    I_OR,
    I_EOR,
    I_AND,
    I_LSH,
    I_SRSH,
    I_URSH,
    I_ADD,
    I_SUB,
    I_MUL,
    I_UDIV,
    I_SDIV,
    I_UMOD,
    I_SMOD,
    I_NEG,
    I_CMPL,
    I_LNEG,
    I_CELL,
    I_FUNC,
    I_CMPEQ,
    I_CMPNEQ,
    I_CMPULT,
    I_CMPUGT,
    I_CMPUGET,
    I_CMPULET,
    I_CMPSGT,
    I_CMPSLT,
    I_CMPSGET,
    I_CMPSLET,
    I_SRINIT,
    I_SRCHECK,
} OpCode;

typedef enum {
    O_NONE,
    O_INT,
    O_STR,
    O_LAB,
} OpndKind;

struct IDescr {
    char *mne;
    OpCode opc;
    OpndKind opnd_kind;
} opcode_table[] = {
    { "ERCHK",  I_ERCHK,    O_INT },
    { "ERSTR",  I_ERSTR,    O_STR },
    { "OER",    I_OER,      O_NONE },
    { "CGER",   I_CGER,     O_NONE },
    { "CALL",   I_CALL,     O_LAB },
    { "R",      I_R,        O_NONE },
    { "RC",     I_RC,       O_NONE },
    { "BF",     I_BF,       O_LAB },
    { "BT",     I_BT,       O_LAB },
    { "SET",    I_SET,      O_NONE },
    { "BR",     I_BR,       O_LAB },
    { "SBTP",   I_SBTP,     O_LAB },
    { "CBTP",   I_CBTP,     O_NONE },
    { "TST",    I_TST,      O_STR },
    { "SRP",    I_SRP,      O_STR },
    { "ID",     I_ID,       O_NONE },
    { "NUM",    I_NUM,      O_NONE },
    { "OCT",    I_OCT,      O_NONE },
    { "HEX",    I_HEX,      O_NONE },
    { "SR",     I_SR,       O_NONE },
    { "CHR",    I_CHR,      O_NONE },
    { "DIG",    I_DIG,      O_NONE },
    { "LET",    I_LET,      O_NONE },
    { "CHRCODE",I_CHRCODE,  O_INT },
    { "NDLBL",  I_NDLBL,    O_LAB },
    { "NDMK",   I_NDMK,     O_INT },
    { "OUTRE",  I_OUTRE,    O_NONE },
    { "DUMP",   I_DUMP,     O_NONE },
    { "SRARG",  I_SRARG,    O_STR },
    { "BRARG",  I_BRARG,    O_NONE },
    { "LBARG",  I_LBARG,    O_INT },
    { "CNTCK",  I_CNTCK,    O_INT },
    { "NXTBR",  I_NXTBR,    O_NONE },
    { "RSTBR",  I_RSTBR,    O_NONE },
    { "CKNDNAM",I_CKNDNAM,  O_LAB },
    { "SAVEP",  I_SAVEP,    O_NONE },
    { "CHASE",  I_CHASE,    O_NONE },
    { "RSTRP",  I_RSTRP,    O_NONE },
    { "CKBR" ,  I_CKBR,     O_NONE },
    { "MOVTO",  I_MOVTO,    O_INT },
    { "MOVTOBASE",I_MOVTOBASE,O_NONE },
    { "OUTCL",  I_OUTCL,    O_NONE },
    { "CKTXT",  I_CKTXT,    O_STR },
    { "CKNUM",  I_CKNUM,    O_NONE },
    { "CKID",   I_CKID,     O_NONE },
    { "CKOCT",  I_CKOCT,    O_NONE },
    { "CKHEX",  I_CKHEX,    O_NONE },
    { "CKSR",   I_CKSR,     O_NONE },
    { "CKCHR",  I_CKCHR,    O_NONE },
    { "CKDIG",  I_CKDIG,    O_NONE },
    { "CKLET",  I_CKLET,    O_NONE },
    { "CKLAB",  I_CKLAB,    O_INT },
    { "OUTSR",  I_OUTSR,    O_STR },
    { "OUTCH",  I_OUTCH,    O_INT },
    { "ONL",    I_ONL,      O_NONE },
    { "GNLB",   I_GNLB,     O_INT },
    { "CLABS",  I_CLABS,    O_NONE },
    { "LOAD",   I_LOAD,     O_LAB },
    { "LOADI",  I_LOADI,    O_INT },
    { "STORE",  I_STORE,    O_LAB },
    { "OR",     I_OR,       O_NONE },
    { "EOR",    I_EOR,      O_NONE },
    { "AND",    I_AND,      O_NONE },
    { "LSH",    I_LSH,      O_NONE },
    { "SRSH",   I_SRSH,     O_NONE },
    { "URSH",   I_URSH,     O_NONE },
    { "ADD",    I_ADD,      O_NONE },
    { "SUB",    I_SUB,      O_NONE },
    { "MUL",    I_MUL,      O_NONE },
    { "UDIV",   I_UDIV,     O_NONE },
    { "SDIV",   I_SDIV,     O_NONE },
    { "UMOD",   I_UMOD,     O_NONE },
    { "SMOD",   I_SMOD,     O_NONE },
    { "NEG",    I_NEG,      O_NONE },
    { "CMPL",   I_CMPL,     O_NONE },
    { "LNEG",   I_LNEG,     O_NONE },
    { "CELL",   I_CELL,     O_INT },
    { "FUNC",   I_FUNC,     O_INT },
    { "CMPEQ",  I_CMPEQ,    O_LAB },
    { "CMPNEQ", I_CMPNEQ,   O_LAB },
    { "CMPULT", I_CMPULT,   O_LAB },
    { "CMPUGT", I_CMPUGT,   O_LAB },
    { "CMPUGET",I_CMPUGET,  O_LAB },
    { "CMPULET",I_CMPULET,  O_LAB },
    { "CMPSGT", I_CMPSGT,   O_LAB },
    { "CMPSLT", I_CMPSLT,   O_LAB },
    { "CMPSGET",I_CMPSGET,  O_LAB },
    { "CMPSLET",I_CMPSLET,  O_LAB },
    { "SRINIT", I_SRINIT,   O_LAB },
    { "SRCHECK",I_SRCHECK,  O_LAB },
    { NULL,     -1,         -1 },
};

struct Loc {
    char *name;
    int loc;
};

struct IRec {
    OpCode opcode;
    union {
        char *str;
        ArithType val;
        struct Loc lab;
    };
} *instructions;
int instr_counter, instr_max;
int line_counter = 1;
char *prog_name, *file_path;
ArithType DUMMY_var;
ArithType *LEVEL_var = &DUMMY_var;
ArithType *TYPE_var = &DUMMY_var;
ArithType *VALUE_var = &DUMMY_var;

struct LabSym {
    char *id;
    int val;
    LabSym *next;
} *label_table[LABTABSIZ];

struct FixUp {
    int loc;
    FixUp *next;
} *fixup_list;

char *get_identifier(char *s, char *buf)
{
    while (isblank(*s))
        ++s;
    if (!isalpha(*s) && *s!='_')
        return NULL;
    *buf++ = *s++;
    while (isalnum(*s))
        *buf++ = *s++;
    *buf = '\0';
    return s;
}

char *get_string(char *s, char *buf)
{
    while (isblank(*s))
        ++s;
    if (*s!='\'' || *(s+1)=='\'')
        return NULL;
    ++s;
    while (*s!='\'' && *s!='\0')
        *buf++ = *s++;
    if (*s != '\'')
        return NULL;
    ++s;
    *buf = '\0';
    return s;
}

char *get_number(char *s, char *buf)
{
    while (isblank(*s))
        ++s;
    if (!isdigit(*s))
        return NULL;
    *buf++ = *s++;
    while (isdigit(*s))
        *buf++ = *s++;
    *buf = '\0';
    return s;
}

void err(char *fmt, ...)
{
    va_list args;

    fprintf(stderr, "%s: %s:%d: error: ", prog_name, file_path, line_counter);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
    exit(EXIT_FAILURE);
}

unsigned hash(char *s)
{
    unsigned hash_val;

    for (hash_val = 0; *s != '\0'; s++)
        hash_val = (unsigned)*s + 31*hash_val;
    return hash_val;
}

int label_value(char *s)
{
    LabSym *np;
    unsigned h;

    h = hash(s)%LABTABSIZ;
    for (np = label_table[h]; np != NULL; np = np->next)
        if (np->id == s)
            return np->val;
    return -1;
}

void parse_label(char *s)
{
    LabSym *np;
    unsigned h;
    char buf[LINEBUFSIZ];

    if ((s=get_identifier(s, buf)) == NULL)
        err("expecting identifier on label line");
    h = hash(buf)%LABTABSIZ;
    for (np = label_table[h]; np != NULL; np = np->next)
        if (strcmp(np->id, buf) == 0)
            break;
    if (np != NULL)
        err("label `%s' redefined", buf);
    np = malloc(sizeof(*np));
    np->id = strtab_new(buf, strlen(buf));
    np->val = instr_counter;
    np->next = label_table[h];
    label_table[h] = np;
}

IRec *new_instr(OpCode opcode)
{
    if (instr_counter >= instr_max) {
        instr_max = instr_max?instr_max*2:64;
        instructions = realloc(instructions, sizeof(instructions[0])*instr_max);
    }
    instructions[instr_counter].opcode = opcode;
    return &instructions[instr_counter++];
}

void parse_instruction(char *s)
{
    int i;
    IRec *ir;
    OpCode opc;
    char mne[32], buf[LINEBUFSIZ];

    if ((s=get_identifier(s, buf)) == NULL)
        err("expecting mnemonic on instruction line");
    for (i = 0; opcode_table[i].mne != NULL; i++)
        if (strcmp(opcode_table[i].mne, buf) == 0)
            break;
    if (opcode_table[i].mne == NULL)
        err("unknown mnemonic `%s'", buf);
    strcpy(mne, buf);
    opc = opcode_table[i].opc;

    switch (opcode_table[i].opnd_kind) {
    case O_LAB: {
        FixUp *fp;

        if (get_identifier(s, buf) == NULL)
            err("instruction `%s' requires a label operand", mne);
        fp = malloc(sizeof(*fp));
        fp->loc = instr_counter;
        fp->next = fixup_list;
        fixup_list = fp;
        ir = new_instr(opc);
        ir->lab.name = strtab_new(buf, strlen(buf));
    }
        break;
    case O_STR:
        if (get_string(s, buf) == NULL)
            err("instruction `%s' requires a string operand", mne);
        ir = new_instr(opc);
        ir->str = strtab_new(buf, strlen(buf));
        break;
    case O_INT:
        if (get_number(s, buf) == NULL)
            err("instruction `%s' requires a number operand", mne);
        ir = new_instr(opc);
        ir->val = strtoul(buf, NULL, 10);
        break;
    default:
        ir = new_instr(opc);
        ir->str = NULL;
        break;
    }
}

void read_program(void)
{
    FILE *fp;
    FixUp *fx, *next;
    char linebuf[LINEBUFSIZ];
    int loc;

    if ((fp=fopen(file_path, "rb")) == NULL) {
        fprintf(stderr, "%s: cannot read file `%s'\n", prog_name, file_path);
        exit(EXIT_FAILURE);
    }
    while (fgets(linebuf, sizeof(linebuf), fp) != NULL) {
        if (linebuf[0] != '\n') {
            if (linebuf[0] == '%')
                parse_label(linebuf+1);
            else
                parse_instruction(linebuf);
        }
        ++line_counter;
    }
    fclose(fp);
    for (fx = fixup_list; fx != NULL; fx = next) {
        IRec *ir;

        ir = &instructions[fx->loc];
        if ((loc=label_value(ir->lab.name)) == -1) {
            fprintf(stderr, "%s: label `%s' referenced but never defined\n", prog_name, ir->lab.name);
            exit(EXIT_FAILURE);
        }
        ir->lab.loc = loc;
        next = fx->next;
        free(fx);
    }
    if ((loc=label_value(strtab_new("LEVEL", 5))) != -1)
        LEVEL_var = &instructions[loc].val;
    if ((loc=label_value(strtab_new("TYPE", 4))) != -1)
        TYPE_var = &instructions[loc].val;
    if ((loc=label_value(strtab_new("VALUE", 5))) != -1)
        VALUE_var = &instructions[loc].val;
}

/* ========================================================================== */
/* EXECUTION                                                                  */
/* ========================================================================== */

char *inbuf;
int label_counter = 1;
Stack *syntax_call_stack, *code_call_stack;

void dump_tree2(int ndx);
void dump_tree(int ndx);

void dump_backtrace(Stack *stk)
{
    int i, n;

    fprintf(stderr, "\n%s rules backtrace:\n", (stk==syntax_call_stack)?"Syntax":"Code");
    for (i=0, n=stack_length(stk); i < n; i++)
        fprintf(stderr, "%*s%s%s\n", i*2, "", (char *)stack_index(stk, i), (i==n-1)?" <-":"");
}

void print_syntax_error(char *inpos, int num, char *str)
{
    char *beg, *end;

    fprintf(stderr, "%s: %s:%d: syntax error `", prog_name, file_path, line_counter);
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
}

void serr(char *fmt, ...)
{
    if (fmt != NULL) {
        va_list args;

        fprintf(stderr, "%s: ", prog_name);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
    dump_backtrace(syntax_call_stack);
    exit(EXIT_FAILURE);
}

void cerr(char *fmt, ...)
{
    if (fmt != NULL) {
        va_list args;

        fprintf(stderr, "%s: ", prog_name);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
    }
    dump_backtrace(code_call_stack);
    exit(EXIT_FAILURE);
}

void badarg(int func)
{
    static char *funcstr[] = {
        "LEN", "CODE", "CONV",
        "XCONV", "OUTL", "OUTC",
        "ENTER", "LOOK", "CLEAR",
        "PUSH", "POP", "OUT",
        "OUTS",
    };
    fprintf(stderr, "%s: invalid argument to function %s\n", prog_name, funcstr[func]);
    cerr(NULL);
}

typedef enum {
    T_TEXT,
    T_NUM,
    T_ID,
    T_OCT,
    T_HEX,
    T_SR,
    T_CHR,
    T_DIG,
    T_LET,
    T_LAB,
    T_Nonterm,
    T_NodPtr,
    T_NumBr,
} CellKind;
#define ISSTRING(x) ((x) <= T_LET)

typedef struct Cell Cell;
typedef struct Frame Frame;

struct Cell {
    CellKind kind;
    union {
        char *text;
        struct Loc outrule;
        int nodptr;
        int nbr;
        int lab;
    };
};

Cell *KSTACK;
int kstack_max, kstack_top = -1;

Cell *kstack_push(CellKind kind)
{
    if (++kstack_top >= kstack_max) {
        kstack_max = kstack_max?kstack_max*2:32;
        KSTACK = realloc(KSTACK, sizeof(KSTACK[0])*kstack_max);
    }
    KSTACK[kstack_top].kind = kind;
    return &KSTACK[kstack_top];
}

Cell *kstack_pop(void)
{
    if (kstack_top < 0) {
        fprintf(stderr, "%s: tree stack underflow\n", prog_name);
        serr(NULL);
    }
    return &KSTACK[kstack_top--];
}

/*
      Nonterminal node layout

    +--------------------------------------+
    | Zero or more branches:               |
    |   -> terminals (T_TEXT, T_ID, ...)   |
    |   -> node pointers (T_NodPtr)        |
    +--------------------------------------+ <- +2 (leftmost branch)
    | # of branches (T_NumBr)              |
    +--------------------------------------+ <- +1
    | Nonterminal node word (T_NonTerm)    |
    +--------------------------------------+ <- +0
*/
Cell *NSTACK;
int nstack_max, nstack_top = -1;

Cell *nstack_push(CellKind kind)
{
    if (++nstack_top >= nstack_max) {
        nstack_max = nstack_max?nstack_max*2:32;
        NSTACK = realloc(NSTACK, sizeof(NSTACK[0])*nstack_max);
    }
    NSTACK[nstack_top].kind = kind;
    return &NSTACK[nstack_top];
}

int currnode, currbr;
struct {
    int currnode;
    int currbr;
} *PSTACK;
int pstack_max, pstack_top = -1;

void pstack_push(void)
{
    if (++pstack_top >= pstack_max) {
        pstack_max = pstack_max?pstack_max*2:32;
        PSTACK = realloc(PSTACK, sizeof(PSTACK[0])*pstack_max);
    }
    PSTACK[pstack_top].currnode = currnode;
    PSTACK[pstack_top].currbr = currbr;
}

struct Frame {
    int labs[4];
    int retloc;
    int currnode;   /* current node when the rule was entered */
    int nstack_top; /* NSTACK's top when the rule was entered */
} *MSTACK;
int mstack_max, mstack_top = -1;

void mstack_push_frame(int retloc)
{
    if (++mstack_top >= mstack_max) {
        mstack_max = mstack_max?mstack_max*2:32;
        MSTACK = realloc(MSTACK, sizeof(MSTACK[0])*mstack_max);
    }
    memset(MSTACK[mstack_top].labs, 0, sizeof(MSTACK[0].labs));
    MSTACK[mstack_top].retloc = retloc;
    MSTACK[mstack_top].currnode = currnode;
    MSTACK[mstack_top].nstack_top = nstack_top;
}

struct {
    char *inpos;
    int kstack_top, nstack_top, mstack_top;
    int line_counter;
    int scs_top;
    int rloc;
} *SSTACK;
int sstack_max, sstack_top = -1;

void sstack_push(char *inpos, int rloc)
{
    if (++sstack_top >= sstack_max) {
        sstack_max = sstack_max?sstack_max*2:32;
        SSTACK = realloc(SSTACK, sizeof(SSTACK[0])*sstack_max);
    }
    SSTACK[sstack_top].inpos = inpos;
    SSTACK[sstack_top].rloc = rloc;
    SSTACK[sstack_top].kstack_top = kstack_top;
    SSTACK[sstack_top].nstack_top = nstack_top;
    SSTACK[sstack_top].mstack_top = mstack_top;
    SSTACK[sstack_top].line_counter = line_counter;
    SSTACK[sstack_top].scs_top = stack_length(syntax_call_stack);
}

int sstack_restore(char **inpos)
{
    *inpos = SSTACK[sstack_top].inpos;
    kstack_top = SSTACK[sstack_top].kstack_top;
    nstack_top = SSTACK[sstack_top].nstack_top;
    mstack_top = SSTACK[sstack_top].mstack_top;
    line_counter = SSTACK[sstack_top].line_counter;
    stack_trim(syntax_call_stack, SSTACK[sstack_top].scs_top);
    return SSTACK[sstack_top].rloc;
}

ArithType *ESTACK;
int estack_max, estack_top = -1;

void estack_push(ArithType v)
{
    if (++estack_top >= estack_max) {
        estack_max = estack_max?estack_max*2:32;
        ESTACK = realloc(ESTACK, sizeof(ESTACK[0])*estack_max);
    }
    ESTACK[estack_top] = v;
}

#define MAX_SYSSTACK 128
ArithType sysstack[MAX_SYSSTACK];
int sysstack_top = -1;

char *skip_white(char *s)
{
    int done;

    done = FALSE;
    while (!done) {
        done = TRUE;
        if (isspace(*s)) {
            do {
                if (*s == '\n')
                    ++line_counter;
                ++s;
            } while (isspace(*s));
            done = FALSE;
        }
        if (s[0]=='/' && s[1]=='*') {
            int sl;

            sl = line_counter;
            s += 2;
            while (s[0]!='\0' && (s[0]!='*' || s[1]!='/'))
                if (*s++ == '\n')
                    ++line_counter;
            if (*s == '\0') {
                fprintf(stderr, "%s: unterminated comment (starts at line %d)\n", prog_name, sl);
                exit(EXIT_FAILURE);
            }
            s += 2;
            done = FALSE;
        }
    }
    return s;
}

void execute(char *inpos)
{
    int i;
    char tokbuf[512];
    char *s, *t, *q;
    int MFLAG;
    int nodptr;
    Stack *tstack;
    IRec *ip, *lim;

    assert(instructions[0].opcode == I_CALL);
    ip = &instructions[instructions[0].lab.loc];
    lim = &instructions[instr_counter];
    stack_push(syntax_call_stack, instructions[0].lab.name);
    tstack = stack_new();

    MFLAG = TRUE;
    mstack_push_frame(-1);

    while (ip < lim) {
        switch (ip->opcode) {
        case I_ERCHK:
        case I_ERSTR:
            if (!MFLAG) {
                if (sstack_top >= 0) {
                    ip = instructions+sstack_restore(&inpos);
                    continue;
                }
                if (ip->opcode == I_ERCHK)
                    print_syntax_error(inpos, ip->val, NULL);
                else
                    print_syntax_error(inpos, 0, ip->str);
                serr(NULL);
            }
            break;
        case I_OER:
            if (!MFLAG)
                cerr("non-first outitem failed");
            break;
        case I_CGER:
            if (!MFLAG)
                serr("code generation failed");
            break;

        case I_CALL:
            stack_push(syntax_call_stack, ip->lab.name);
            mstack_push_frame((ip-instructions)+1);
            ip = instructions+ip->lab.loc;
            continue;
        case I_R:
            if (mstack_top == 0)
                goto done;
            ip = instructions+MSTACK[mstack_top].retloc;
            --mstack_top;
            stack_pop(syntax_call_stack);
            continue;
        case I_RC:
            ip = instructions+MSTACK[mstack_top].retloc;
            --mstack_top;
            nstack_top = MSTACK[mstack_top].nstack_top;
            currnode = MSTACK[mstack_top].currnode;
            stack_pop(code_call_stack);
            continue;
        case I_BF:
            if (!MFLAG) {
                ip = instructions+ip->lab.loc;
                continue;
            }
            break;
        case I_BT:
            if (MFLAG) {
                ip = instructions+ip->lab.loc;
                continue;
            }
            break;
        case I_SET:
            MFLAG = TRUE;
            break;
        case I_BR:
            ip = instructions+ip->lab.loc;
            continue;
        case I_SBTP:
            sstack_push(inpos, ip->lab.loc);
            break;
        case I_CBTP:
            --sstack_top;
            break;

        case I_SRARG:
            kstack_push(T_TEXT)->text = ip->str;
            break;
        case I_LBARG:
            assert(ip->val>=1 && ip->val<=4);
            if (!MSTACK[mstack_top].labs[ip->val-1])
                MSTACK[mstack_top].labs[ip->val-1] = label_counter++;
            kstack_push(T_LAB)->lab = MSTACK[mstack_top].labs[ip->val-1];
            break;
        case I_BRARG:
            if (NSTACK[currbr].kind == T_LAB)
                cerr("using nodename to access label argument");
            if (NSTACK[currbr].kind == T_NodPtr)
                kstack_push(T_NodPtr)->nodptr = NSTACK[currbr].nodptr;
            else
                kstack_push(NSTACK[currbr].kind)->text = NSTACK[currbr].text;
            break;
        case I_NDLBL:
            kstack_push(T_Nonterm)->outrule = ip->lab;
            break;
        case I_NDMK:
            if (kstack_top<0 || KSTACK[kstack_top].kind!=T_Nonterm)
                serr("node-building operation [%d] is not preceded by a name definition", ip->val);
            nstack_push(T_Nonterm)->outrule = kstack_pop()->outrule;
            nodptr = nstack_top;
            nstack_push(T_NumBr)->nbr = ip->val;
            for (i = 0; i < ip->val; i++)
                stack_push(tstack, kstack_pop());
            while (!stack_isempty(tstack)) {
                Cell *x;

                x = stack_pop(tstack);
                switch (x->kind) {
                case T_NodPtr:
                    nstack_push(T_NodPtr)->nodptr = x->nodptr;
                    break;
                case T_Nonterm: /* something like :A :B[1] */
                    serr("trying to add nonterminal as branch of tree");
                    break;
                default:
                    nstack_push(x->kind)->text = x->text;
                    break;
                }
            }
            kstack_push(T_NodPtr)->nodptr = nodptr;
            break;
        case I_OUTRE:
            if (kstack_top < 0)
                break;
            pstack_top = -1;
            if (KSTACK[kstack_top].kind != T_NodPtr) {
                assert(KSTACK[kstack_top].kind != T_LAB);
                printf("%s", kstack_pop()->text);
                MFLAG = TRUE;
                break;
            }
            currnode = kstack_pop()->nodptr;
            currbr = currnode+2;
            mstack_push_frame((ip-instructions)+1);
            ip = instructions+NSTACK[currnode].outrule.loc;
            stack_push(code_call_stack, NSTACK[currnode].outrule.name);
            continue;
        case I_OUTCL:
            pstack_top = -1;
            if (NSTACK[currbr].kind != T_NodPtr) {
                if (NSTACK[currbr].kind == T_LAB)
                    cerr("using nodename to access label argument");
                printf("%s", NSTACK[currbr].text);
                currnode = MSTACK[mstack_top].currnode;
                MFLAG = TRUE;
                break;
            }
            currnode = NSTACK[currbr].nodptr;
            currbr = currnode+2;
            mstack_push_frame((ip-instructions)+1);
            ip = instructions+NSTACK[currnode].outrule.loc;
            stack_push(code_call_stack, NSTACK[currnode].outrule.name);
            continue;
        case I_DUMP:
            if (kstack_top < 0)
                break;
            dump_tree(kstack_pop()->nodptr);
            pstack_top = nstack_top = kstack_top = -1;
            MFLAG = TRUE;
            break;

        case I_CNTCK:
            assert(NSTACK[currnode+1].kind == T_NumBr);
            MFLAG = NSTACK[currnode+1].nbr==ip->val;
            break;
        case I_NXTBR:
            assert(NSTACK[currnode+1].kind == T_NumBr);
            ++currbr;
            assert((currbr-currnode-2) < NSTACK[currnode+1].nbr);
            break;
        case I_RSTBR:
            currbr = currnode+2;
            break;
        case I_CKNDNAM:
            if (NSTACK[currbr].kind == T_NodPtr)
                MFLAG = NSTACK[NSTACK[currbr].nodptr].outrule.name==ip->lab.name;
            else
                MFLAG = FALSE;
            break;
        case I_SAVEP:
            pstack_push();
            break;
        case I_RSTRP:
            assert(pstack_top >= 0);
            currnode = PSTACK[pstack_top].currnode;
            currbr = PSTACK[pstack_top].currbr;
            --pstack_top;
            break;
        case I_CHASE:
            if (NSTACK[currbr].kind != T_NodPtr)
                cerr("attempt to access a non-existent branch");
            currnode = NSTACK[currbr].nodptr;
            currbr = currnode+2;
            break;
        case I_CKBR:
            assert(pstack_top >= 0);
            i = PSTACK[pstack_top].currbr;
            if (ISSTRING(NSTACK[i].kind) && ISSTRING(NSTACK[currbr].kind))
                MFLAG = NSTACK[i].text==NSTACK[currbr].text;
            else
                MFLAG = FALSE;
            break;
        case I_MOVTOBASE:
            currnode = MSTACK[mstack_top].currnode;
            currbr = currnode+2;
            break;
        case I_MOVTO:
            assert(NSTACK[currnode+1].kind == T_NumBr);
            if (ip->val > NSTACK[currnode+1].nbr)
                cerr("attempt to access a non-existent branch");
            currbr = currnode+2+(ip->val-1);
            break;
        case I_CKTXT:
            MFLAG = ISSTRING(NSTACK[currbr].kind) && NSTACK[currbr].text==ip->str;
            break;
        case I_CKNUM:
            MFLAG = NSTACK[currbr].kind==T_NUM;
            break;
        case I_CKID:
            MFLAG = NSTACK[currbr].kind==T_ID;
            break;
        case I_CKOCT:
            MFLAG = NSTACK[currbr].kind==T_OCT;
            break;
        case I_CKHEX:
            MFLAG = NSTACK[currbr].kind==T_HEX;
            break;
        case I_CKSR:
            MFLAG = NSTACK[currbr].kind==T_SR;
            break;
        case I_CKCHR:
            MFLAG = NSTACK[currbr].kind==T_CHR
                 || NSTACK[currbr].kind==T_DIG
                 || NSTACK[currbr].kind==T_LET;
            break;
        case I_CKDIG:
            MFLAG = NSTACK[currbr].kind==T_DIG
                 || NSTACK[currbr].kind==T_CHR && isdigit(*NSTACK[currbr].text);
            break;
        case I_CKLET:
            MFLAG = NSTACK[currbr].kind==T_LET
                 || NSTACK[currbr].kind==T_CHR && isalpha(*NSTACK[currbr].text);
            break;
        case I_CKLAB:
            /*
                We set label slots only when the item
                is an outermost item (pstack_top is -1).
            */
            if ((MFLAG = (NSTACK[currbr].kind==T_LAB)) && pstack_top==-1)
                MSTACK[mstack_top].labs[ip->val-1] = NSTACK[currbr].lab;
            break;
        case I_CLABS:
            if (!MFLAG)
                memset(MSTACK[mstack_top].labs, 0, sizeof(MSTACK[0].labs));
            break;

        case I_OUTSR:
            printf("%s", ip->str);
            break;
        case I_OUTCH:
            printf("%c", ip->val);
            break;
        case I_ONL:
            printf("\n");
            break;
        case I_GNLB:
            assert(ip->val>=1 && ip->val<=4);
            if (!MSTACK[mstack_top].labs[ip->val-1])
                MSTACK[mstack_top].labs[ip->val-1] = label_counter++;
            printf("L%d", MSTACK[mstack_top].labs[ip->val-1]);
            break;

        case I_TST:
        case I_SRP:
            inpos = skip_white(inpos);
            for (s=inpos, t=ip->str; *t!='\0' && *s==*t; s++, t++)
                ;
            if (*t == '\0') {
                inpos = s;
                if (ip->opcode == I_SRP)
                    kstack_push(T_TEXT)->text = ip->str;
                MFLAG = TRUE;
            } else {
                MFLAG = FALSE;
            }
            break;
        case I_NUM:
            inpos = skip_white(inpos);
            if (isdigit(*inpos)) {
                q = tokbuf;
                do
                    *q++ = *inpos++;
                while (isdigit(*inpos));
                *q = '\0';
                kstack_push(T_NUM)->text = strtab_new(tokbuf, q-tokbuf);
                MFLAG = TRUE;
            } else {
                MFLAG = FALSE;
            }
            break;
        case I_ID:
            inpos = skip_white(inpos);
            if (isalpha(*inpos)) {
                q = tokbuf;
                do
                    *q++ = *inpos++;
                while (isalnum(*inpos));
                *q = '\0';
                kstack_push(T_ID)->text = strtab_new(tokbuf, q-tokbuf);
                MFLAG = TRUE;
            } else {
                MFLAG = FALSE;
            }
            break;
        case I_OCT:
            inpos = skip_white(inpos);
            if (*inpos>='0' && *inpos<='7') {
                q = tokbuf;
                do
                    *q++ = *inpos++;
                while (*inpos>='0' && *inpos<='7');
                *q = '\0';
                kstack_push(T_OCT)->text = strtab_new(tokbuf, q-tokbuf);
                MFLAG = TRUE;
            } else {
                MFLAG = FALSE;
            }
            break;
        case I_HEX:
            inpos = skip_white(inpos);
            if (isxdigit(*inpos)) {
                q = tokbuf;
                do
                    *q++ = *inpos++;
                while (isxdigit(*inpos));
                *q = '\0';
                kstack_push(T_HEX)->text = strtab_new(tokbuf, q-tokbuf);
                MFLAG = TRUE;
            } else {
                MFLAG = FALSE;
            }
            break;
        case I_SR:
            inpos = skip_white(inpos);
            if (*(s=inpos) == '\'') {
                ++s;
                q = tokbuf;
                while (*s!='\'' && *s!='\0' && *s!='\n')
                    *q++ = *s++;
            }
            if (*s == '\'') {
                *q = '\0';
                inpos = ++s;
                kstack_push(T_SR)->text = strtab_new(tokbuf, q-tokbuf);
                MFLAG = TRUE;
            } else {
                MFLAG = FALSE;
            }
            break;
        case I_CHR:
            tokbuf[0] = *inpos++;
            tokbuf[1] = '\0';
            kstack_push(T_CHR)->text = strtab_new(tokbuf, 1);
            MFLAG = TRUE;
            break;
        case I_DIG:
            inpos = skip_white(inpos);
            if (isdigit(*inpos)) {
                tokbuf[0] = *inpos++;
                tokbuf[1] = '\0';
                kstack_push(T_DIG)->text = strtab_new(tokbuf, 1);
                MFLAG = TRUE;
            } else {
                MFLAG = FALSE;
            }
            break;
        case I_LET:
            inpos = skip_white(inpos);
            if (isalpha(*inpos)) {
                tokbuf[0] = *inpos++;
                tokbuf[1] = '\0';
                kstack_push(T_LET)->text = strtab_new(tokbuf, 1);
                MFLAG = TRUE;
            } else {
                MFLAG = FALSE;
            }
            break;
        case I_CHRCODE:
            inpos = skip_white(inpos);
            if (*inpos == (unsigned char)ip->val) {
                ++inpos;
                MFLAG = TRUE;
            } else {
                MFLAG = FALSE;
            }
            break;

        case I_LOAD:
            estack_push(instructions[ip->lab.loc].val);
            break;
        case I_LOADI:
            estack_push(ip->val);
            break;
        case I_STORE:
            instructions[ip->lab.loc].val = ESTACK[estack_top--];
            break;

        case I_OR:
            ESTACK[estack_top-1] |= ESTACK[estack_top];
            --estack_top;
            break;
        case I_EOR:
            ESTACK[estack_top-1] ^= ESTACK[estack_top];
            --estack_top;
            break;
        case I_AND:
            ESTACK[estack_top-1] &= ESTACK[estack_top];
            --estack_top;
            break;
        case I_LSH:
            ESTACK[estack_top-1] <<= ESTACK[estack_top];
            --estack_top;
            break;
        case I_SRSH:
            ((SignedArithType *)ESTACK)[estack_top-1] >>= ESTACK[estack_top];
            --estack_top;
            break;
        case I_URSH:
            ESTACK[estack_top-1] >>= ESTACK[estack_top];
            --estack_top;
            break;
        case I_ADD:
            ESTACK[estack_top-1] += ESTACK[estack_top];
            --estack_top;
            break;
        case I_SUB:
            ESTACK[estack_top-1] -= ESTACK[estack_top];
            --estack_top;
            break;
        case I_MUL:
            ESTACK[estack_top-1] *= ESTACK[estack_top];
            --estack_top;
            break;
        case I_UDIV:
            ESTACK[estack_top-1] /= ESTACK[estack_top];
            --estack_top;
            break;
        case I_SDIV:
            ((SignedArithType *)ESTACK)[estack_top-1] /= (SignedArithType)ESTACK[estack_top];
            --estack_top;
            break;
        case I_UMOD:
            ESTACK[estack_top-1] %= ESTACK[estack_top];
            --estack_top;
            break;
        case I_SMOD:
            ((SignedArithType *)ESTACK)[estack_top-1] %= (SignedArithType)ESTACK[estack_top];
            --estack_top;
            break;
        case I_NEG:
            ESTACK[estack_top] = -ESTACK[estack_top];
            break;
        case I_CMPL:
            ESTACK[estack_top] = ~ESTACK[estack_top];
            break;
        case I_LNEG:
            ESTACK[estack_top] = !ESTACK[estack_top];
            break;

        case I_CMPEQ:
            MFLAG = instructions[ip->lab.loc].val==ESTACK[estack_top--];
            break;
        case I_CMPNEQ:
            MFLAG = instructions[ip->lab.loc].val!=ESTACK[estack_top--];
            break;
        case I_CMPULT:
            MFLAG = instructions[ip->lab.loc].val<ESTACK[estack_top--];
            break;
        case I_CMPUGT:
            MFLAG = instructions[ip->lab.loc].val>ESTACK[estack_top--];
            break;
        case I_CMPUGET:
            MFLAG = instructions[ip->lab.loc].val>=ESTACK[estack_top--];
            break;
        case I_CMPULET:
            MFLAG = instructions[ip->lab.loc].val<=ESTACK[estack_top--];
            break;
        case I_CMPSGT:
            MFLAG = (SignedArithType)instructions[ip->lab.loc].val>(SignedArithType)ESTACK[estack_top--];
            break;
        case I_CMPSLT:
            MFLAG = (SignedArithType)instructions[ip->lab.loc].val<(SignedArithType)ESTACK[estack_top--];
            break;
        case I_CMPSGET:
            MFLAG = (SignedArithType)instructions[ip->lab.loc].val>=(SignedArithType)ESTACK[estack_top--];
            break;
        case I_CMPSLET:
            MFLAG = (SignedArithType)instructions[ip->lab.loc].val<=(SignedArithType)ESTACK[estack_top--];
            break;

        case I_FUNC:
            if (ip->val <= 7) {
                if (!ISSTRING(NSTACK[currbr].kind))
                    badarg(ip->val);
                switch (ip->val) {
                case 0: /* LEN */
                    estack_push(strlen(NSTACK[currbr].text));
                    break;
                case 1: /* CODE */
                    estack_push((unsigned char)*NSTACK[currbr].text);
                    break;
                case 2: /* CONV */
                    if (NSTACK[currbr].kind != T_NUM)
                        badarg(ip->val);
                    estack_push((ArithType)strtoul(NSTACK[currbr].text, NULL, 10));
                    break;
                case 3: /* XCONV */
                    if (NSTACK[currbr].kind != T_HEX)
                        badarg(ip->val);
                    estack_push((ArithType)strtoul(NSTACK[currbr].text, NULL, 16));
                    break;
                case 4: /* OUTL */
                    printf("%u", strlen(NSTACK[currbr].text));
                    break;
                case 5: /* OUTC */
                    printf("%c", *NSTACK[currbr].text);
                    break;
                case 6: /* ENTER */
                    symtab_enter(NSTACK[currbr].text, *LEVEL_var, *TYPE_var, *VALUE_var);
                    break;
                case 7: /* LOOK */
                    MFLAG = symtab_look(NSTACK[currbr].text, LEVEL_var, TYPE_var, VALUE_var);
                    break;
                }
            } else {
                switch (ip->val) {
                case 8: /* CLEAR */
                    ESTACK[estack_top] = symtab_clear(ESTACK[estack_top]);
                    break;
                case 9: /* PUSH */
                    if (++sysstack_top >= MAX_SYSSTACK)
                        cerr("system stack overflow");
                    sysstack[sysstack_top] = ESTACK[estack_top--];
                    break;
                case 10: /* POP */
                    if (sysstack_top < 0)
                        cerr("system stack underflow");
                    estack_push(sysstack[sysstack_top--]);
                    break;
                case 11: /* OUT */
                    printf("%u", ESTACK[estack_top--]);
                    break;
                case 12: /* OUTS */
                    printf("%d", ESTACK[estack_top--]);
                    break;
                default:
                    assert(0);
                }
            }
            break;

        case I_SRINIT: {
            char *name;

            if ((name=symtab_iterate(TRUE, LEVEL_var, TYPE_var, VALUE_var)) != NULL) {
                nstack_push(T_TEXT)->text = name;
                MSTACK[mstack_top].nstack_top = nstack_top;
                NSTACK[currnode+1].nbr = 1;
            } else {
                ip = instructions+ip->lab.loc;
                continue;
            }
        }
            break;
        case I_SRCHECK: {
            char *name;

            if ((name=symtab_iterate(FALSE, LEVEL_var, TYPE_var, VALUE_var)) != NULL) {
                assert(NSTACK[currnode+1].kind == T_NumBr);
                NSTACK[currnode+2].text = name;
                ip = instructions+ip->lab.loc;
                continue;
            }
        }
            break;

        case I_CELL:
        default:
            assert(0);
        }
        ++ip;
    }
done:
    stack_destroy(tstack);
}

void dump_tree2(int ndx)
{
    static char *opstr[] = {
        "String", ".NUM", ".ID",
        ".OCT", ".HEX", ".SR",
        ".CHR", ".DIG", ".LET",
    };
    static int vertex_counter;

    if (NSTACK[ndx].kind == T_Nonterm) {
        int i, v;

        v = vertex_counter++;
        printf("V%d[label=\"%s\"]\n", v, NSTACK[ndx].outrule.name);
        assert(NSTACK[ndx+1].kind == T_NumBr);
        for (i = 0; i < NSTACK[ndx+1].nbr; i++) {
            printf("V%d -> V%d\n", v, vertex_counter);
            dump_tree2(ndx+2+i);
        }
    } else if (NSTACK[ndx].kind == T_NodPtr) {
        dump_tree2(NSTACK[ndx].nodptr);
    } else {
        assert(NSTACK[ndx].kind <= T_LET);
        printf("V%d[label=\"%s\\n(`%s')\"]\n", vertex_counter++, opstr[NSTACK[ndx].kind], NSTACK[ndx].text);
    }
}

void dump_tree(int ndx)
{
    printf("digraph {\n");
    dump_tree2(ndx);
    printf("}\n");
}

int main(int argc, char *argv[])
{
    FILE *fp;
    unsigned len;

    prog_name = argv[0];
    if (argc < 3) {
        fprintf(stderr, "usage: %s <code> <input>\n", prog_name);
        exit(EXIT_SUCCESS);
    }
    file_path = argv[1];
    read_program();

    file_path = argv[2];
    if ((fp=fopen(file_path, "rb")) == NULL) {
        fprintf(stderr, "%s: cannot read file `%s'\n", prog_name, file_path);
        exit(EXIT_FAILURE);
    }
    fseek(fp, 0, SEEK_END);
    len = ftell(fp);
    rewind(fp);
    inbuf = malloc(len+1);
    len = fread(inbuf, 1, len, fp);
    inbuf[len] = '\0';
    fclose(fp);
    line_counter = 1;
    syntax_call_stack = stack_new();
    code_call_stack = stack_new();
    execute(inbuf);

    return 0;
}
