#ifndef NIKOMSTR_H
#include "/Include/NiKomStr.h"
#endif

#ifndef NIKOMLIB_H
#include "/Include/NiKomLib.h"
#endif

#ifndef DOS_DOS_H
#include <dos/dos.h>
#endif

/* Funktionsprototyper */

/* De i librariet */
void __saveds __asm LIBMatrix2NiKom(register __a6 struct NiKomBase *);
LONG __saveds __asm LIBRexxEntry(register __a0 struct RexxMsg *,register __a6 struct NiKomBase *);
void __saveds __asm LIBLockNiKomBase(register __a6 struct NiKomBase *);
void __saveds __asm LIBUnLockNiKomBase(register __a6 struct NiKomBase *);
struct FidoText * __saveds __asm LIBReadFidoText(register __a0 char *, register __a1 struct TagItem *,register __a6 struct NiKomBase *);
void __saveds __asm LIBFreeFidoText(register __a0 struct FidoText *);
int __saveds __asm LIBWriteFidoText(register __a0 struct FidoText *, register __a1 struct TagItem *,register __a6 struct NiKomBase *);
void __saveds __asm LIBReScanFidoConf(register __a0 struct Mote *, register __d0 int, register __a6 struct NiKomBase *);
void __saveds __asm LIBUpdateFidoConf(register __a0 struct Mote *, register __a6 struct NiKomBase *);
void __saveds __asm LIBUpdateAllFidoConf(register __a6 struct NiKomBase *);
void __saveds __asm LIBReScanAllFidoConf(register __a6 struct NiKomBase *);
struct NodeType * __saveds __asm LIBGetNodeType(register __d0 long, register __a6 struct NiKomBase *);
int __saveds __asm LIBReNumberConf(register __a0 struct Mote *, register __d0 int,
	register __d1 int, register __a6 struct NiKomBase *);
int __saveds __asm LIBWriteConf(register __a0 struct Mote *, register __a6 struct NiKomBase *);
struct Mote * __saveds __asm LIBGetConfPoint(register __d0 int, register __a6 struct NiKomBase *);
int __saveds __asm LIBMaySeeConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds __asm LIBMayBeMemberConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds __asm LIBMayReadConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds __asm LIBMayWriteConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds __asm LIBMayReplyConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds __asm LIBMayAdminConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
int __saveds __asm LIBIsMemberConf(register __d0 int, register __d1 int, register __a0 struct User *,register __a6 struct NiKomBase *);
void __saveds __asm LIBGetNiKomVersion(register __a0 int *, register __a1 int *,	register __a2 char *);
void __saveds __asm LIBConvChrsToAmiga(register __a0 char *, register __d0 int,
	register __d1 int, register __a6 struct NiKomBase *);
void __saveds __asm LIBConvChrsFromAmiga(register __a0 char *, register __d0 int,
	register __d1 int, register __a6 struct NiKomBase *);
void __saveds __asm LIBStripAnsiSequences(register __a0 char *, register __a6 struct NiKomBase *);
int __saveds __asm LIBSetNodeState(register __d0 int, register __d1 int);
int __saveds __asm LIBSendNodeMessage(register __d0 int, register __d1 int, register __a0 char *, register __a6 struct NiKomBase *);
NiKHash * __saveds __asm LIBNewNiKHash(register __d0 int);
void __saveds __asm LIBDeleteNiKHash(register __a0 NiKHash *);
int __saveds __asm LIBInsertNiKHash(register __a0 NiKHash *, register __d0 int, register __a1 void *);
void * __saveds __asm LIBGetNiKHashData(register __a0 NiKHash *, register __d0 int);
void * __saveds __asm LIBRemoveNiKHashData(register __a0 NiKHash *, register __d0 int);
int __saveds __asm LIBCreateUser(register __d0 LONG, register __a0 struct TagItem *, register __a6 struct NiKomBase *);

int __saveds __asm LIBNiKParse(register __a0 char *string, register __d0 char subject, register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBSysInfo(register __a0 char *subject, register __a6 struct NiKomBase *NiKomBase);

int __saveds __asm LIBAddProgramData( register __d0 int, register __a1 char *, register __a2 char *, register __a3 char *, register __a6 struct NiKomBase *);
char __saveds __asm *LIBGetProgramData( register __d0 int, register __a0 char *, register __a1 char *, register __a2 char *, register __a6 struct NiKomBase *);
int __saveds __asm LIBFreeProgramCategory( register __d0 int usernumber, register __a6 struct NiKomBase *NiKomBase);
int __saveds __asm LIBLoadProgramCategory( register __d0 int, register __a6 struct NiKomBase *);
int __saveds __asm LIBSaveProgramCategory( register __d0 int, register __a6 struct NiKomBase *);

/* Andra trevliga små funktioner */

/* Matrix.c */
int getlastmatrix(struct NiKomBase *);
void debug_req(char *,APTR);
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
int gethwm(char *);
int sethwm(char *,int);

void rexxdebugreq(struct RexxMsg *);

/* Echo.c */
struct Mote *getmotpek(int, struct System *);

/* terminal.c */
UBYTE convnokludge(UBYTE);

/* ServerComm.c */

struct MsgPort *SafePutToPort(struct NiKMess *, char *);
long sendservermess(short, long, long, long, long);
int linksaystring(int, int, char *, struct NiKomBase *);

/* PrgCat.c */
int InsertProgramCategory(char *, struct ProgramCategory **);
int InsertProgramData(char *, char *, char *, struct ProgramCategory *);
struct ProgramCategory *GetCategorypek(char *, struct ProgramCategory *);
int DeleteCategory(char *, struct ProgramCategory **);
void DeleteProgramData(struct ProgramData *);
void DeleteAll(struct ProgramCategory **);
int LoadAll(char *, struct ProgramCategory **);
int SaveAll(int, struct ProgramCategory *);
char *InternGetProgramData(char *, char *, struct ProgramCategory *);

struct ProgramDataCache *CheckifUserisCached(int, struct NiKomBase *);
struct ProgramCategory *CacheProgramDataforUser(int, struct NiKomBase *);
void InsertProgramDataCachedUser(int, struct ProgramCategory *, struct NiKomBase *);
void RemoveFirstCachedUser(struct NiKomBase *);
