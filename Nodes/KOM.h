#include "NiKomStr.h"

extern struct Stack *g_unreadRepliesStack;
extern struct IntList *g_readRepliesList;

void KomLoop(void);
int FindNextUnreadConf(int currentConfId);
void ExecuteCommand(struct Kommando *cmd);
void ExecuteCommandById(int cmdId);
void GoConf(int confId);
