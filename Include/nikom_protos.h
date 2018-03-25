#ifndef NIKOM_PROTOS_H
#define NIKOM_PROTOS_H

#include <proto/exec.h>
#include "NiKomStr.h"

extern struct Library *NiKomBase;

LONG RexxEntry(struct RexxMsg *);
void LockNiKomBase(void);
void UnLockNiKomBase(void);

void Matrix2NiKom(void);
struct FidoText *ReadFidoText(char *, struct TagItem *);
struct FidoText *ReadFidoTextTags(char *, unsigned long tag1Type, ... );
void FreeFidoText(struct FidoText *);
int WriteFidoText(struct FidoText *,struct TagItem *);
int WriteFidoTextTags(struct FidoText *,unsigned long tag1Type, ... );
void ReScanFidoConf(struct Mote *,int);
void UpdateFidoConf(struct Mote *);
void UpdateAllFidoConf(void);
void ReScanAllFidoConf(void);
struct NodeType *GetNodeType(long);
int ReNumberConf(struct Mote *, int, int);
int WriteConf(struct Mote *);
void InitServermem(struct System *);
struct Mote *GetConfPoint(int);
int MaySeeConf(int, int, struct User *);
int MayBeMemberConf(int, int, struct User *);
int MayReadConf(int, int, struct User *);
int MayWriteConf(int, int, struct User *);
int MayReplyConf(int, int, struct User *);
int MayAdminConf(int, int, struct User *);
int IsMemberConf(int, int, struct User *);
char *GetBuildTime(void);
void ConvChrsToAmiga(char *, int, int);
void ConvChrsFromAmiga(char *, int, int);
int ConvMBChrsToAmiga(char *, char *, int, int);
int ConvMBChrsFromAmiga(char *, char *, int, int, int);
int SetNodeState(int, int);
/* NiKHash *NewNiKHash(int);
void DeleteNiKHash(NiKHash *);
int InsertNiKHash(NiKHash *, int, void *);
void *GetNiKHashData(NiKHash *, int);
void *RemoveNiKHashData(NiKHash *, int); */
int NiKParse(char *, char);
void ChangeUnreadTextStatus(
  int textNumber, int markAsUnread, struct UnreadTexts *unreadTexts);
int IsTextUnread(int textNumber, struct UnreadTexts *unreadTexts);
void InitUnreadTexts(struct UnreadTexts *unreadTexts);
int FindNextUnreadText(int searchStart, int conf, struct UnreadTexts *unreadTexts);
int CountUnreadTexts(int conf, struct UnreadTexts *unreadTexts);
void SetUnreadTexts(int conf, int amount, struct UnreadTexts *unreadTexts);
int ReadUnreadTexts(struct UnreadTexts *unreadTexts, int userId);
int WriteUnreadTexts(struct UnreadTexts *unreadTexts, int userId);

/* Crypt.c */

int CheckPassword(char *, char *);
char *CryptPassword(char *, char *);

/* ConferenceTexts.c */
int GetConferenceForText(int textNumber);
void SetConferenceForText(int textNumber, int conf, int saveToDisk);
int FindNextTextInConference(int searchStart, int conf);
int FindPrevTextInConference(int searchStart, int conf);
int WriteConferenceTexts(void);
int DeleteConferenceTexts(int numberOfTexts);
#endif /* NIKOM_PROTOS_H */
