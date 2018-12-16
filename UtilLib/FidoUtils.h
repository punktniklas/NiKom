int ParseFidoAddress(char *str, int result[]);
int IsZoneInStr(int zone, char *str, int emptyMatchesAll);
char *MakeMsgFilePath(char *dir, int msgId, char *buf);
int GetNextMsgNum(char *dir);
void SetNextMsgNum(char *dir, int msgNum);
