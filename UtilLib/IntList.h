struct IntList;

struct IntList *CreateIntList(int initialSize);
void DeleteIntList(struct IntList *list);
int IntListInsert(struct IntList *list, int index, int value);
int IntListAppend(struct IntList *list, int value);
int IntListSize(struct IntList *list);
int IntListGet(struct IntList *list, int index);
int IntListRemoveIndex(struct IntList *list, int index);
int IntListFind(struct IntList *list, int value);
int IntListRemoveValue(struct IntList *list, int value);
void IntListClear(struct IntList *list);
void IntListDebugPrint(struct IntList *list, char *label);
