#ifndef SYMTAB_H_
#define SYMTAB_H_

#include "arithtype.h"

void symtab_enter(char *name, ArithType LEVEL, ArithType TYPE, ArithType VALUE);
int symtab_look(char *name, ArithType *LEVEL, ArithType *TYPE, ArithType *VALUE);
char *symtab_iterate(int begin, ArithType *LEVEL, ArithType *TYPE, ArithType *VALUE);
int symtab_clear(ArithType low);

#endif
