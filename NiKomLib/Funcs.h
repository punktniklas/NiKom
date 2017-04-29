#ifndef NIKOMSTR_H
#include "NiKomStr.h"
#endif

#ifndef NIKOMLIB_H
#include "NiKomLib.h"
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

#include "NiKomCompat.h"

#include "NiKomBase.h"

/* Function prototypes */

/* Those in the library */
void __saveds AASM LIBMatrix2NiKom(register __a6 struct NiKomBase *);
LONG __saveds AASM LIBRexxEntry(register __a0 struct RexxMsg *,register __a6 struct NiKomBase *);
void __saveds AASM LIBLockNiKomBase(register __a6 struct NiKomBase *);
void __saveds AASM LIBUnLockNiKomBase(register __a6 struct NiKomBase *);
struct FidoText * __saveds AASM LIBReadFidoText(register __a0 char *, register __a1 struct TagItem *,register __a6 struct NiKomBase *);
void __saveds AASM LIBFreeFidoText(register __a0 struct FidoText *);
int __saveds AASM LIBWriteFidoText(register __a0 struct FidoText *, register __a1 struct TagItem *,register __a6 struct NiKomBase *);
void __saveds AASM LIBReScanFidoConf(register __a0 struct Mote *, register __d0 int, register __a6 struct NiKomBase *);
void __saveds AASM LIBUpdateFidoConf(register __a0 struct Mote *, register __a6 struct NiKomBase *);
void __saveds AASM LIBUpdateAllFidoConf(register __a6 struct NiKomBase *);
void __saveds AASM LIBReScanAllFidoConf(register __a6 struct NiKomBase *);
struct NodeType * __saveds AASM LIBGetNodeType(register __d0 long, register __a6 struct NiKomBase *);
int __saveds AASM LIBReNumberConf(register __a0 struct Mote *, register __d0 int,
	register __d1 int, register __a6 struct NiKomBase *);
int __saveds AASM LIBWriteConf(register __a0 struct Mote *, register __a6 struct NiKomBase *);
struct Mote * __saveds AASM LIBGetConfPoint(register __d0 int, register __a6 struct NiKomBase *);
int __saveds AASM LIBMaySeeConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds AASM LIBMayBeMemberConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds AASM LIBMayReadConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds AASM LIBMayWriteConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds AASM LIBMayReplyConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds AASM LIBMayAdminConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds AASM LIBIsMemberConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
char * __saveds AASM LIBGetBuildTime();
void __saveds AASM LIBConvChrsToAmiga(register __a0 char *, register __d0 int,
	register __d1 int, register __a6 struct NiKomBase *);
void __saveds AASM LIBConvChrsFromAmiga(register __a0 char *, register __d0 int,
	register __d1 int, register __a6 struct NiKomBase *);
void __saveds AASM LIBStripAnsiSequences(register __a0 char *, register __a6 struct NiKomBase *);
int __saveds AASM LIBConvMBChrsToAmiga(register __a0 char *, register __a1 char *,
	register __d0 int, register __d1 int, register __a6 struct NiKomBase *);
int __saveds AASM LIBConvMBChrsFromAmiga(register __a0 char *, register __a1 char *,
	register __d0 int, register __d1 int, register __a6 struct NiKomBase *);
int __saveds AASM LIBSetNodeState(register __d0 int, register __d1 int);
int __saveds AASM LIBSendNodeMessage(register __d0 int, register __d1 int, register __a0 char *, register __a6 struct NiKomBase *);
NiKHash * __saveds AASM LIBNewNiKHash(register __d0 int);
void __saveds AASM LIBDeleteNiKHash(register __a0 NiKHash *);
int __saveds AASM LIBInsertNiKHash(register __a0 NiKHash *, register __d0 int, register __a1 void *);
void * __saveds AASM LIBGetNiKHashData(register __a0 NiKHash *, register __d0 int);
void * __saveds AASM LIBRemoveNiKHashData(register __a0 NiKHash *, register __d0 int);
int __saveds AASM LIBCreateUser(register __d0 LONG, register __a0 struct TagItem *, register __a6 struct NiKomBase *);

int __saveds AASM LIBNiKParse(register __a0 char *string, register __d0 char subject, register __a6 struct NiKomBase *NiKomBase);
int __saveds AASM LIBSysInfo(register __a0 char *subject, register __a6 struct NiKomBase *NiKomBase);

/* Other useful little functions */

/* Matrix.c */
int sprattmatchar(char *,char *);
int fidoparsenamn(char *,struct System *);
int updatenextletter(int);
void writelog(char *,char *);

/* ReadFidoText.c */
int getfidoline(char *, char *, int, int, BPTR,char *,struct NiKomBase *);
char *hittaefter(char *);
int getzone(char *);
int getnet(char *);
int getnode(char *);
int getpoint(char *);
int gethwm(char *, char);
int sethwm(char *,int, char);

/* Echo.c */
struct Mote *getmotpek(int, struct System *);

/* Terminal.c */
UBYTE convnokludge(UBYTE);

/* ServerComm.c */

struct MsgPort *SafePutToPort(struct NiKMess *, char *);
long sendservermess(short, long, long, long, long);
int linksaystring(int, int, char *, struct NiKomBase *);
