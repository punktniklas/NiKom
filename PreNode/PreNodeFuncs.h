#include <devices/serial.h>

#include <NiKomNodeCommon.h>
/* Prototypes for functions defined in Ser.c */
void cleanup(int kod,char *text);

/* Modem.c */
void disconnect(void);
void waitconnect(void);

/* Terminal.c */
int getkeyfile(void);

/* ServerComm.c */
int initnode(int);
void shutdownnode(int);
void handleservermess(struct NiKMess *);

/* Prototypes for functions defined in Rexx.c */
int sendrexx(int komnr);
void rexxgetstring(struct RexxMsg *tempmess);
void rexxsendstring(struct RexxMsg *);
void rexxgettekn(struct RexxMsg *mess);
void rexxchkbuffer(struct RexxMsg *mess);
void rexxyesno(struct RexxMsg *mess);
void rxsetlinecount(struct RexxMsg *mess);
void rxsendchar(struct RexxMsg *mess);
void rxsendrawfile(struct RexxMsg *mess);

/* Prototypes for functions defined in Stuff.c */
char *hittaefter(char *);
int parsenamn(char *);
int matchar(char *, char *);
int readuser(int,struct User *);
char *getusername(int);
void NiKForbid(void);
void NiKPermit(void);
