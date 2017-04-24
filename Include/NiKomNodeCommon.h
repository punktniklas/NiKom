#ifndef NIKOMNODECOMMON_H
#define NIKOMNODECOMMON_H

#include <intuition/intuition.h>
/*
 * TODO: The long time goal is for this file to die. Every .c
 * file should be a module that should have a corresponding .h
 * file that contains the public functions of that module.
 */

/*
 * Function prototypes for functions shared between Nodes and PreNode.
 */

/* Prototypes for functions defined in SerialIO.c */
int OpenIO(struct Window *);
void CloseIO(void);
BYTE OpenConsole(struct Window *window);
BYTE OpenSerial(struct IOExtSer *writereq, struct IOExtSer *readreq,
                struct IOExtSer *changereq);
BYTE OpenTimer(struct timerequest *treq);
void CloseConsole(void);
void CloseSerial(struct IOExtSer *writereq);
void CloseTimer(struct timerequest *treq);
void writesererr(int);
int DoSerIOErr(struct IOExtSer *req);
char gettekn(void);
char congettkn(void);
char sergettkn(void);
void eka(char tecken);
void sereka(char tecken);
void coneka(char tecken);
void conreqtkn(void);
void serreqtkn(void);
void putstring(char *pekare, int size, long flags);
int tknwaiting(void);
void getnodeconfig(char *);
int getfifoevent(struct MsgPort *fifoport, char *puthere);
void abortserial(void);
int checkcharbuffer(void);
int puttekn(char *pekare,int size);
int sendtosercon(char *conpek, char *serpek, int consize, int sersize);
int sendtocon(char *pekare, int);
int serputtekn(char *pekare,int size);
int sendtoser(char *pekare, int);

#endif /* NIKOMSTR_H */
