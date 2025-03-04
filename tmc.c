#include <stdio.h>
#include "input.h"
#include "syntax.h"

char *prog_name;
int MFLAG = TRUE;

int main(int argc, char *argv[])
{
    prog_name = argv[0];
    if (argc < 3) {
        fprintf(stderr, "usage: %s <code> <input>\n", prog_name);
        return 0;
    }
    parse(read_input(argv[1]), argv[2]);

    return 0;
}
