void sendfile(char *filename);
int GetString(int maxchrs, char *defaultStr);
int GetSecretString(int maxchrs, char *defaultStr);
int getstring(int echo, int maxchrs, char *defaultStr);
int GetStringX(int echo, int maxchrs, char *defaultStr,
               int (*isCharacterAccepted)(unsigned char),
               int (*isStringAccepted)(char *, void *),
               void *customData);
int GetNumber(int minvalue, int maxvalue, char *defaultStr);
int SendString(char *fmt, ...);
int SendStringNoBrk(char *fmt, ...);
int conputtekn(char *pekare,int size);
void DisplayInternalError(void);
int GetYesOrNo(char *label, char yesChar, char noChar, char *yesStr, char *noStr,
               int yesIsDefault, int *res);
int MaybeEditString(char *label, char *str, int maxlen);
int MaybeEditPassword(char *label1, char *label2, char *pwd, int maxlen);
int MaybeEditNumber(char *label, int *number, int maxlen, int minVal, int maxVal);
int MaybeEditNumberChar(char *label, char *number, int maxlen, int minVal,
                        int maxVal);
