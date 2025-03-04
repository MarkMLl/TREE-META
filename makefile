.PHONY: all clean

CC=gcc
CFLAGS=-c -g -Wall -Wconversion -Wno-switch -Wno-parentheses -Wno-sign-conversion
TMC=tmc
TMM=tmm
SRCS=strtab.c symtab.c stack.c tmc.c input.c syntax.c code.c tmm.c
UTIL_OBJS=strtab.o symtab.o stack.o
TMC_OBJS=tmc.o input.o syntax.o code.o $(UTIL_OBJS)
TMM_OBJS=tmm.o $(UTIL_OBJS)
DEPFILE=make.depend

all: $(TMC) $(TMM)

$(TMC): $(TMC_OBJS)
	$(CC) -o $(TMC) $(TMC_OBJS)

$(TMM): $(TMM_OBJS)
	$(CC) -o $(TMM) $(TMM_OBJS)

.c.o:
	$(CC) $(CFLAGS) $*.c

clean:
	rm -f *.o $(TMC) $(TMM) $(DEPFILE) examples/*.output treemeta1.tmm treemeta2.tmm

test: $(TMC) $(TMM)
	@ /bin/bash test1.sh
	@ /bin/bash test2.sh

meta: $(TMC) $(TMM)
	./$(TMC) treemeta.tm treemeta.tm >treemeta1.tmm
	./$(TMM) treemeta1.tmm treemeta.tm >treemeta2.tmm
	cmp treemeta1.tmm treemeta2.tmm

depend:
	@ $(CC) $(CFLAGS) -MM $(SRCS) >$(DEPFILE)

-include $(DEPFILE)
