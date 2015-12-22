#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>

#include "Stack.h"

#define STACK_ALLOC_SIZE 100

struct Stack {
  int size, arraySize;
  int *data;
};

struct Stack *CreateStack(void) {
  struct Stack *stack;
  if((stack = AllocMem(sizeof(struct Stack), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    return NULL;
  }
  stack->size = 0;
  stack->arraySize = STACK_ALLOC_SIZE;
  if((stack->data = AllocMem(STACK_ALLOC_SIZE * sizeof(int), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    FreeMem(stack, sizeof(struct Stack));
    return NULL;
  }
  return stack;
}

void DeleteStack(struct Stack *stack) {
  FreeMem(stack->data, stack->arraySize * sizeof(int));
  FreeMem(stack, sizeof(struct Stack));
}

int growStack(struct Stack *stack) {
  int newSize;
  int *newData;

  newSize = stack->arraySize * 2;
  if((newData = AllocMem(newSize * sizeof(int), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    return 0;
  }
  memcpy(newData, stack->data, stack->arraySize * sizeof(int));
  FreeMem(stack->data, stack->arraySize * sizeof(int));
  stack->data = newData;
  stack->arraySize = newSize;
  return 1;
}

void StackPush(struct Stack *stack, int value) {
  if(stack->size == stack->arraySize) {
    if(!growStack(stack)) {
      return;
    }
  }
  stack->data[stack->size++] = value;
}

int StackPop(struct Stack *stack) {
  if(stack->size == 0) {
    return 0;
  }
  return stack->data[--stack->size];
}

int StackPeek(struct Stack *stack) {
  if(stack->size == 0) {
    return 0;
  }
  return stack->data[stack->size - 1];
}

int StackSize(struct Stack *stack) {
  return stack->size;
}

void StackClear(struct Stack *stack) {
  stack->size = 0;
}
