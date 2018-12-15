#include <stdio.h>
#include <stdlib.h>

#include "BTree.h"

void main(int argc, char *argv[]) {
  struct BTree *tree;
  int block;
  char input[20];
  if(argc != 2) {
    printf("Usage: DebugBTree <path>\n");
    return;
  }
  if((tree = BTreeOpen(argv[1])) == NULL) {
    printf("Can not open BTree at path '%s'\n", argv[1]);
    return;
  }
  BTreeSetDebugMode(1);
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
