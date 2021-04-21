char *FindStringCfgValue(char *str);
int GetStringCfgValue(char *str, char *dest, int len);
int GetLongCfgValue(char *str, long *value, int lineCnt);
int GetShortCfgValue(char *str, short *value, int lineCnt);
int GetCharCfgValue(char *str, char *value, int lineCnt);
int GetBoolCfgFlag(char *str, long *flagfield, long flag, int lineCnt);
