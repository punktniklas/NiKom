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
void __saveds AASM LIBMatrix2NiKom(register __a6 struct NiKomBase * AREG(a6));
LONG __saveds AASM LIBRexxEntry(register __a0 struct RexxMsg * AREG(a0),register __a6 struct NiKomBase * AREG(a6));
void __saveds AASM LIBLockNiKomBase(register __a6 struct NiKomBase * AREG(a6));
void __saveds AASM LIBUnLockNiKomBase(register __a6 struct NiKomBase * AREG(a6));
struct FidoText * __saveds AASM LIBReadFidoText(register __a0 char * AREG(a0), register __a1 struct TagItem * AREG(a1),register __a6 struct NiKomBase * AREG(a6));
void __saveds AASM LIBFreeFidoText(register __a0 struct FidoText * AREG(a0));
int __saveds AASM LIBWriteFidoText(register __a0 struct FidoText * AREG(a0), register __a1 struct TagItem * AREG(a1),register __a6 struct NiKomBase * AREG(a6));
void __saveds AASM LIBReScanFidoConf(register __a0 struct Mote * AREG(a0), register __d0 int AREG(d0), register __a6 struct NiKomBase * AREG(a6));
void __saveds AASM LIBUpdateFidoConf(register __a0 struct Mote * AREG(a0), register __a6 struct NiKomBase * AREG(a6));
void __saveds AASM LIBUpdateAllFidoConf(register __a6 struct NiKomBase * AREG(a6));
void __saveds AASM LIBReScanAllFidoConf(register __a6 struct NiKomBase * AREG(a6));
struct NodeType * __saveds AASM LIBGetNodeType(register __d0 long AREG(d0), register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBReNumberConf(register __a0 struct Mote * AREG(a0), register __d0 int AREG(d0),
	register __d1 int AREG(d1), register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBWriteConf(register __a0 struct Mote * AREG(a0), register __a6 struct NiKomBase * AREG(a6));
struct Mote * __saveds AASM LIBGetConfPoint(register __d0 int AREG(d0), register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBMaySeeConf(register __d0 int AREG(d0), register __d1 int AREG(d1), register __a0 struct User * AREG(a0),register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBMayBeMemberConf(register __d0 int AREG(d0), register __d1 int AREG(d1), register __a0 struct User * AREG(a0),register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBMayReadConf(register __d0 int AREG(d0), register __d1 int AREG(d1), register __a0 struct User * AREG(a0),register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBMayWriteConf(register __d0 int AREG(d0), register __d1 int AREG(d1), register __a0 struct User * AREG(a0),register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBMayReplyConf(register __d0 int AREG(d0), register __d1 int AREG(d1), register __a0 struct User * AREG(a0),register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBMayAdminConf(register __d0 int AREG(d0), register __d1 int AREG(d1), register __a0 struct User * AREG(a0),register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBIsMemberConf(register __d0 int AREG(d0), register __d1 int AREG(d1), register __a0 struct User * AREG(a0),register __a6 struct NiKomBase * AREG(a6));
char * __saveds AASM LIBGetBuildTime();
void __saveds AASM LIBConvChrsToAmiga(register __a0 char * AREG(a0), register __d0 int AREG(d0),
	register __d1 int AREG(d1), register __a6 struct NiKomBase * AREG(a6));
void __saveds AASM LIBConvChrsFromAmiga(register __a0 char * AREG(a0), register __d0 int AREG(d0),
	register __d1 int AREG(d1), register __a6 struct NiKomBase * AREG(a6));
void __saveds AASM LIBStripAnsiSequences(register __a0 char * AREG(a0), register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBConvMBChrsToAmiga(register __a0 char * AREG(a0), register __a1 char * AREG(a1),
	register __d0 int AREG(d0), register __d1 int AREG(d1), register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBConvMBChrsFromAmiga(register __a0 char * AREG(a0),
                                         register __a1 char * AREG(a1),
                                         register __d0 int AREG(d0),
                                         register __d1 int AREG(d1),
                                         register __d2 int AREG(d2),
                                         register __a6 struct NiKomBase * AREG(a6));
int __saveds AASM LIBSetNodeState(register __d0 int AREG(d0), register __d1 int AREG(d1));
int __saveds AASM LIBSendNodeMessage(register __d0 int AREG(d0), register __d1 int AREG(d1), register __a0 char * AREG(a0), register __a6 struct NiKomBase * AREG(a6));
NiKHash * __saveds AASM LIBNewNiKHash(register __d0 int AREG(d0));
void __saveds AASM LIBDeleteNiKHash(register __a0 NiKHash * AREG(a0));
int __saveds AASM LIBInsertNiKHash(register __a0 NiKHash * AREG(a0), register __d0 int AREG(d0), register __a1 void * AREG(a1));
void * __saveds AASM LIBGetNiKHashData(register __a0 NiKHash * AREG(a0), register __d0 int AREG(d0));
void * __saveds AASM LIBRemoveNiKHashData(register __a0 NiKHash * AREG(a0), register __d0 int AREG(d0));
int __saveds AASM LIBCreateUser(register __d0 LONG AREG(d0), register __a0 struct TagItem * AREG(a0), register __a6 struct NiKomBase * AREG(a6));

int __saveds AASM LIBNiKParse(register __a0 char *string AREG(a0), register __d0 char subject AREG(d0), register __a6 struct NiKomBase *NiKomBase AREG(a6));
int __saveds AASM LIBSysInfo(register __a0 char *subject AREG(a0), register __a6 struct NiKomBase *NiKomBase AREG(a6));

int __saveds AASM LIBCheckPassword(
  register __a0 char *clearText AREG(a0),
  register __a1 char *correctPassword AREG(a1),
  register __a6 struct NiKomBase *NiKomBase AREG(a6));
char *__saveds AASM LIBCryptPassword(
  register __a0 char *clearText AREG(a0),
  register __a1 char *resultBuf AREG(a1),
  register __a6 struct NiKomBase *NiKomBase AREG(a6));

void __saveds AASM LIBInitServermem(
  register __a0 struct System *Servermem AREG(a0),
  register __a6 struct NiKomBase *NiKomBase AREG(a6));


/* Other useful little functions */

/* Matrix.c */
int sprattmatchar(char *,char *);
int fidoparsenamn(char *,struct System *);
int updatenextletter(int);
void writelog(char *,char *);

/* ReadFidoText.c */
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
int convUTF8ToAmiga(char *dst, const char *src, unsigned len);

/* ServerComm.c */

struct MsgPort *SafePutToPort(struct NiKMess *, char *);
long sendservermess(short, long, long, long, long);
int linksaystring(int, int, char *, struct NiKomBase *);
