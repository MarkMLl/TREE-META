#include "strtab.h"
#include <string.h>
#include <assert.h>
#include <stdlib.h>

#define TAB_SIZE 4093

typedef struct StrNode StrNode;

static struct StrNode {
    int len;
    char *str;
    StrNode *next;
} *strtab[TAB_SIZE];
static int nstr, nbytes;

static unsigned hash(const char *s)
{
    unsigned hash_val;

    for (hash_val = 0; *s != '\0'; s++)
        hash_val = (unsigned)*s + 31*hash_val;
    return hash_val;
}

char *strtab_new(const char *str, int len)
{
    unsigned h;
    StrNode *np;

    assert(str != NULL);
    assert(len >= 0);
    h = hash(str)%TAB_SIZE;
    for (np = strtab[h]; np != NULL; np = np->next)
        if (np->len==len && memcmp(np->str, str, len)==0)
            return np->str;
    np = malloc(sizeof(*np)+len+1);
    np->len = len;
    np->str = (char *)(np+1);
    if (len > 0)
        memcpy(np->str, str, len);
    np->str[len] = '\0';
    np->next = strtab[h];
    strtab[h] = np;
    ++nstr;
    nbytes += len+1;
    return np->str;
}
