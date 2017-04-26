#include "NiKomStr.h"

extern struct Stack *g_unreadRepliesStack;

void KomLoop(void);
int FindNextUnreadConf(int currentConfId);
void DoExecuteCommand(struct Kommando *cmd);
void GoConf(int confId);
