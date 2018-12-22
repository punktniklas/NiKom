#include <exec/memory.h>
#include <proto/exec.h>
#include <string.h>

#include "IntList.h"

struct IntList {
  int size, arraySize;
  int *array;
};

struct IntList *CreateIntList(int initialSize) {
  struct IntList *list;
  if((list = AllocMem(sizeof(struct IntList), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    return NULL;
  }

  list->size = 0;
  list->arraySize = initialSize;
  if((list->array = AllocMem(initialSize * sizeof(int), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    FreeMem(list, sizeof(struct IntList));
    return NULL;
  }

  return list;
}

void DeleteIntList(struct IntList *list) {
  FreeMem(list->array, list->arraySize * sizeof(int));
  FreeMem(list, sizeof(struct IntList));
}

static int growList(struct IntList *list) {
  int newSize = list->arraySize * 2;
  int *newArray;

  if((newArray = AllocMem(newSize * sizeof(int), MEMF_CLEAR | MEMF_PUBLIC)) == NULL) {
    return 0;
  }
  memcpy(newArray, list->array, list->arraySize * sizeof(int));
  FreeMem(list->array, list->arraySize * sizeof(int));
  list->array = newArray;
  list->arraySize = newSize;
  return 1;
}

int IntListInsert(struct IntList *list, int index, int value) {
  int i;
  if(list->size == list->arraySize) {
    if(!growList(list)) {
      return 0;
    }
  }
  for(i = list->size; i > index; i--) {
    list->array[i] = list->array[i - 1];
  }
  list->array[index] = value;
  list->size++;
  return 1;
}

int IntListAppend(struct IntList *list, int value) {
  return IntListInsert(list, list->size, value);
}

int IntListSize(struct IntList *list) {
  return list->size;
}

int IntListGet(struct IntList *list, int index) {
  return list->array[index];
}

int IntListRemoveIndex(struct IntList *list, int index) {
  int value, i;
  value = list->array[index];
  for(i = index; i < list->size - 1; i++) {
    list->array[i] = list->array[i + 1];
  }
  list->size--;
  return value;
}

int IntListFind(struct IntList *list, int value) {
  int i;
  for(i = 0; i < list->size; i++) {
    if(list->array[i] == value) {
      return i;
    }
  }
  return -1;
}

int IntListRemoveValue(struct IntList *list, int value) {
  int index;
  index = IntListFind(list, value);
  if(index == -1) {
    return -1;
  }
  IntListRemoveIndex(list, index);
  return index;
}

void IntListClear(struct IntList *list) {
  list->size = 0;
}
