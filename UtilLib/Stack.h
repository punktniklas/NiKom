struct Stack;

struct Stack *CreateStack(void);
void DeleteStack(struct Stack *stack);
int growStack(struct Stack *stack);
void StackPush(struct Stack *stack, int value);
int StackPop(struct Stack *stack);
int StackPeek(struct Stack *stack);
int StackSize(struct Stack *stack);
