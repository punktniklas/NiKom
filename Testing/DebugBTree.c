#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "BTree.h"

void main(int argc, char *argv[]) {
  struct BTree *tree;
  int block, keyFormat;
  char input[20];
  if(argc != 3) {
    printf("Usage: DebugBTree <path> <key format>\n");
    printf("       key format is either 'string' or 'int'\n");
    return;
  }
  if(strcmp(argv[2], "string") == 0) {
    keyFormat = BTREE_DBG_KEY_STRING;
  } else if(strcmp(argv[2], "int") == 0) {
    keyFormat = BTREE_DBG_KEY_INT;
  } else {
    printf("Invalid key format.\n");
    return;
  }

  if((tree = BTreeOpen(argv[1])) == NULL) {
    printf("Can not open BTree at path '%s'\n", argv[1]);
    return;
  }
  BTreeSetDebugMode(keyFormat);
  BTreePrintRoot(tree);
  for(;;) {
    printf("Which node? ");
    gets(input);
    if(input[0] == 'q') {
      break;
    }
    block = atoi(input);
    printf("\nPrinting node %d\n", block);
    BTreePrintNodeFromBlock(tree, block);
  }
  printf("Bye bye!\n");
  BTreeClose(tree);
}
