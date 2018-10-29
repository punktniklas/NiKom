struct DiskMem;

int CreateDiskMem(char *path, int blockSize);
struct DiskMem *OpenDiskMem(char *path);
void CloseDiskMem(struct DiskMem *diskMem);
int GetDiskMemBlockSize(struct DiskMem *diskMem);
int AllocateDiskMemBlock(struct DiskMem *diskMem);
void FreeDiskMemBlock(struct DiskMem *diskMem, int block);
int ReadDiskMemBlock(struct DiskMem *diskMem, int block, void *buffer);
int WriteDiskMemBlock(struct DiskMem *diskMem, int block, void *buffer);
