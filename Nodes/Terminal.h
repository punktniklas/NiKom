void sendfile(char *filename);
int GetChar(void);
int IsPrintableCharacter(unsigned char c);
int GetString(int maxchrs, char *defaultStr);
int GetSecretString(int maxchrs, char *defaultStr);
int getstring(int echo, int maxchrs, char *defaultStr);
int GetStringX(int echo, int maxchrs, char *defaultStr,
               int (*isCharacterAccepted)(unsigned char),
               int (*isStringAccepted)(char *, void *),
               void *customData);
int GetNumber(int minvalue, int maxvalue, char *defaultStr);
int SendString(char *fmt, ...);
int SendStringCat(char *fmt, char *catStr, ...);
int SendStringNoBrk(char *fmt, ...);
int SendRepeatedChr(char c, int count);
int conputtekn(char *pekare,int size);
void DisplayInternalError(void);
int GetYesOrNo(char *preStr, char *label,
               char *yesChar, char *noChar, char *yesStr, char *noStr,
               char *postStr, int yesIsDefault, int *res);
int EditString(char *label, char *str, int maxlen, int nonEmpty);
int MaybeEditString(char *label, char *str, int maxlen);
int MaybeEditPassword(char *label1, char *label2, char *pwd, int maxlen);
int MaybeEditNumber(char *label, int *number, int maxlen, int minVal, int maxVal);
int MaybeEditNumberChar(char *label, char *number, int maxlen, int minVal,
                        int maxVal);
int EditBitFlag(char *label, char yesChar, char noChar, char *yesStr, char *noStr,
                long *value, long bitmask);
int EditBitFlagShort(char *label, char yesChar, char noChar,
                     char *yesStr, char *noStr, short *value, long bitmask);
int EditBitFlagChar(char *label, char yesChar, char noChar,
                    char *yesStr, char *noStr, char *value, long bitmask);

#define GETCHAR_LOGOUT     -1
#define GETCHAR_RETURN     -2
#define GETCHAR_SOL        -3 // Start of line
#define GETCHAR_EOL        -4 // End of line
#define GETCHAR_BACKSPACE  -5
#define GETCHAR_DELETE     -6
#define GETCHAR_DELETELINE -7
#define GETCHAR_UP         -8
#define GETCHAR_DOWN       -9
#define GETCHAR_RIGHT      -10
#define GETCHAR_LEFT       -11
