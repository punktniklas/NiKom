/* Prototypes for functions defined in Ser.c */
BYTE OpenConsole(struct Window *window);
BYTE OpenSerial(struct IOExtSer *writereq,struct IOExtSer *readreq,struct IOExtSer *changereq);
void CloseConsole(void);
void CloseSerial(struct IOExtSer *);
void writesererr(int);
char gettekn(void);
char congettkn(void);
char sergettkn(void);
void eka(char tecken);
void coneka(char tecken);
void conreqtkn(void);
void serreqtkn(void);
void putstring(char *pekare,int size, long flags);
int puttekn(char *pekare,int size);
int conputtekn(char *pekare,int size);
void modemcmd(char *pekare,int size);
void sendfile(char *filnamn);
struct MsgPort *SafePutToPort(struct NiKMess *message,char *portname);
int getstring(int eko,int maxtkn, char *);
int carrierdropped(void);
void getnodeconfig(char *);
char convseventoeight(char foo);
char conveighttoseven(char foo);
void convstring(char *string);
void sendat(char *atstring);
void sendplus(void);
int updateinactive(void);
void freealiasmem(void);
void waitconnect(void);
void cleanup(int kod,char *text);
void main(int argc,char **argv);
void abortinactive(void);
void disconnect(void);

/* SerialIO.c */
int OpenIO(struct Window *);
void CloseIO(void);
void abortserial(void);
int tknwaiting(void);

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
int jaellernej(char,char,int);
int speciallogin(char);
int nyanv(void);
int parsenamn(char *);
int matchar(char *, char *);
int readuser(int,struct User *);
char *getusername(int);
void NiKForbid(void);
void NiKPermit(void);
int bytteckenset(void);
