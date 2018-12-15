struct BTree;

int BTreeCreate(char *path, int keysPerNode, int keySize, int valueSize);
struct BTree *BTreeOpen(char *path);
void BTreeClose(struct BTree *tree);
int BTreeInsert(struct BTree *tree, void *key, void *value);
int BTreeRemove(struct BTree *tree, void *key);
int BTreeGet(struct BTree *tree, void *key, void *valueBuf);
void BTreePrintRoot(struct BTree *tree);
void BTreePrintNodeFromBlock(struct BTree *tree, int block);
void BTreeSetDebugMode(int debug);
