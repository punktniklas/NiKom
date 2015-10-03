void sendfile(char *filename);
int getstring(int echo, int maxchrs, char *defaultStr);
int GetStringX(int echo, int maxchrs, char *defaultStr,
               int (*isCharacterAccepted)(unsigned char),
               int (*isStringAccepted)(char *, void *),
               void *customData);
int GetNumber(int minvalue, int maxvalue, char *defaultStr);
int SendString(char *fmt, ...);
int SendStringNoBrk(char *fmt, ...);
int conputtekn(char *pekare,int size);
