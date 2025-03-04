#include "symtab.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

typedef struct Entry Entry;
typedef struct Level Level;

static struct Entry {
    char *name;
    ArithType TYPE, VALUE;
    Entry *next;
} *entry_free_list;

static struct Level {
    ArithType LEVEL;
    Entry *entries;
    Level *next;
} *symtab, *level_free_list;

static Entry *get_entry_node(void)
{
    Entry *n;

    if ((n=entry_free_list) == NULL) {
        n = calloc(1, sizeof(*n));
    } else {
        entry_free_list = n->next;
        memset(n, 0, sizeof(*n));
    }
    return n;
}

static Level *get_level_node(void)
{
    Level *n;

    if ((n=level_free_list) == NULL) {
        n = calloc(1, sizeof(*n));
    } else {
        level_free_list = n->next;
        memset(n, 0, sizeof(*n));
    }
    return n;
}

void symtab_enter(char *name, ArithType LEVEL, ArithType TYPE, ArithType VALUE)
{
    Entry *n;
    Level *p, *q;

    for (p=symtab, q=NULL; p!=NULL && p->LEVEL<LEVEL; q=p, p=p->next)
        ;
    if (p == NULL) {
        p = get_level_node();
        p->LEVEL = LEVEL;
        p->entries = (n = get_entry_node());
        if (q != NULL)
            q->next = p;
        else
            symtab = p;
    } else if (p->LEVEL == LEVEL) {
        for (n = p->entries; n != NULL; n = n->next)
            if (n->name == name)
                break;
        if (n == NULL) {
            n = get_entry_node();
            n->next = p->entries;
            p->entries = n;
        }
    } else {
        if (q != NULL) {
            q->next = get_level_node();
            q = q->next;
        } else {
            q = symtab = get_level_node();
        }
        q->LEVEL = LEVEL;
        q->entries = (n = get_entry_node());
        q->next = p;
    }
    n->name = name;
    n->TYPE = TYPE;
    n->VALUE = VALUE;
}

int symtab_look(char *name, ArithType *LEVEL, ArithType *TYPE, ArithType *VALUE)
{
    Level *p;
    Entry *n, *res;

    res = NULL;
    for (p = symtab; p != NULL; p = p->next) {
        for (n = p->entries; n != NULL; n = n->next) {
            if (n->name == name) {
                res = n;
                *LEVEL = p->LEVEL;
                break;
            }
        }
    }
    if (res != NULL) {
        *TYPE = res->TYPE;
        *VALUE = res->VALUE;
        return 1;
    }
    return 0;
}

int symtab_clear(ArithType low)
{
    int nent;
    Level *p, *q;

    for (p=symtab, q=NULL; p!=NULL && p->LEVEL<low; q=p, p=p->next)
        ;
    if (p == NULL)
        return 0;
    if (q != NULL)
        q->next = NULL;
    else
        symtab = NULL;
    q = p;
    for (nent = 0;; p = p->next) {
        Entry *n;

        assert(p->entries != NULL);
        for (n = p->entries;; n = n->next) {
            ++nent;
            if (n->next == NULL)
                break;
        }
        n->next = entry_free_list;
        entry_free_list = p->entries;
        if (p->next == NULL)
            break;
    }
    p->next = level_free_list;
    level_free_list = q;
    return nent;
}

char *symtab_iterate(int begin, ArithType *LEVEL, ArithType *TYPE, ArithType *VALUE)
{
    static Level *p;
    static Entry *n;
    char *name;

    if (begin) {
        if ((p=symtab) == NULL)
            return NULL;
        n = p->entries;
    }
    if (p == NULL)
        return NULL;
    if (n == NULL) {
        if ((p=p->next) == NULL)
            return NULL;
        n = p->entries;
    }
    *LEVEL = p->LEVEL;
    *TYPE = n->TYPE;
    *VALUE = n->VALUE;
    name = n->name;
    n = n->next;
    return name;
}
