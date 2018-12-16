struct BTree;

#define BTREE_DBG_KEY_STRING 1
#define BTREE_DBG_KEY_INT    2

int BTreeCreate(char *path, int keysPerNode, int keySize, int valueSize);
struct BTree *BTreeOpen(char *path);
void BTreeClose(struct BTree *tree);
int BTreeInsert(struct BTree *tree, void *key, void *value);
int BTreeRemove(struct BTree *tree, void *key);
int BTreeGet(struct BTree *tree, void *key, void *valueBuf);
void BTreePrintRoot(struct BTree *tree);
void BTreePrintNodeFromBlock(struct BTree *tree, int block);
void BTreeSetDebugMode(int keyFormat);
