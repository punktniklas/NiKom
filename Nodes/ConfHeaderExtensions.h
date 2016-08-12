struct MemHeaderExtensionNode {
  struct MinNode node;
  struct HeaderExtension ext;
  short index;
  char dirty;
};

struct MemHeaderExtension {
  struct MinList nodes;
  long textId;
  char notInHeaderYet;
};

struct MemHeaderExtension *CreateMemHeaderExtension(long textId);
void DeleteMemHeaderExtension(struct MemHeaderExtension *ext);
struct MemHeaderExtensionNode *AddMemHeaderExtensionNode(struct MemHeaderExtension *ext);
int SaveHeaderExtension(struct MemHeaderExtension *ext);
struct MemHeaderExtension *ReadHeaderExtension(int textId, int index);
