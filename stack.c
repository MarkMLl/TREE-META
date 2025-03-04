#include "stack.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define STACK_INIT 16

struct Stack {
    void **stack;
    int top, max;
};

Stack *stack_new(void)
{
    Stack *s;

    s = malloc(sizeof(*s));
    s->stack = malloc(sizeof(s->stack[0])*STACK_INIT);
    s->max = STACK_INIT;
    s->top = -1;
    return s;
}

Stack *stack_dup(Stack *s)
{
    Stack *q;

    q = malloc(sizeof(*q));
    q->stack = malloc(sizeof(q->stack[0])*s->max);
    q->max = s->max;
    if ((q->top=s->top) >= 0)
        memcpy(q->stack, s->stack, sizeof(s->stack[0])*(s->top+1));
    return q;
}

Stack *stack_cpy(Stack *dest, Stack *src)
{
    assert(dest->max >= src->max);
    if ((dest->top=src->top) >= 0)
        memcpy(dest->stack, src->stack, sizeof(src->stack[0])*(src->top+1));
    return dest;
}

void stack_destroy(Stack *s)
{
    free(s->stack);
    free(s);
}

void *stack_push(Stack *s, void *p)
{
    if (++s->top >= s->max) {
        s->max *= 2;
        s->stack = realloc(s->stack, sizeof(s->stack[0])*s->max);
    }
    return (s->stack[s->top] = p);
}

void *stack_pop(Stack *s)
{
    assert(s->top >= 0);
    return s->stack[s->top--];
}

void *stack_peek(Stack *s)
{
    assert(s->top >= 0);
    return s->stack[s->top];
}

int stack_isempty(Stack *s)
{
    return s->top < 0;
}

int stack_length(Stack *s)
{
    return s->top+1;
}

void stack_trim(Stack *s, int n)
{
    assert(n-1 <= s->top);
    s->top = n-1;
}

void *stack_index(Stack *s, unsigned i)
{
    assert(i <= s->top);
    return s->stack[i];
}
