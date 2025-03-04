#ifndef STACK_H_
#define STACK_H_

typedef struct Stack Stack;

Stack *stack_new(void);
Stack *stack_dup(Stack *s);
Stack *stack_cpy(Stack *dest, Stack *src);
void stack_destroy(Stack *s);
void *stack_push(Stack *s, void *p);
void *stack_pop(Stack *s);
void *stack_peek(Stack *s);
int stack_isempty(Stack *s);
int stack_length(Stack *s);
void *stack_index(Stack *s, unsigned i);
void stack_trim(Stack *s, int n);

#endif
