#include "NiKomCompat.h"
#include <dos/dos.h>
#include <dos/exall.h>
#include <exec/memory.h>
#include <rexx/storage.h>
#include <proto/exec.h>
#ifdef HAVE_PROTO_ALIB_H
/* For NewList() */
# include <proto/alib.h>
#endif
#include <proto/intuition.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKVersion.h"
#include "Config.h"
#include "VersionStrings.h"
#include "Shutdown.h"

#include "Startup.h"

#define EXIT_ERROR	     10
#define OK	     0
#define MINOSVERSION 37

void openLibrariesAndPorts(void);
void setupServermem(void);
void readTextInfo(void);
void readGroupData(void);
void readConferenceData(void);
void readConferenceTexts(void);
void readUserData(void);
void readLastLogins(void);
void readFileAreas(void);
void readFileAreaFiles(void);
void readSysInfo(void);
void scanFidoConferences(void);
int maybeConvertConferenceTextData(int numberOfTexts);
void readIntoConfTextsArray(int arrayIndex, int fileIndex, int textsToRead,
                            BPTR file);
void initNodes(void);
void initLanguages(void);
void openWindow(void);

struct IntuitionBase *IntuitionBase;
struct RsxLib *RexxSysBase=NULL;
struct Library *UtilityBase;
struct Library *NiKomBase;
struct MsgPort *NiKPort, *permitport, *rexxport, *nodereplyport;
char pubscreen[40], NiKomReleaseStr[50], windowTitle[100];
int xpos, ypos;
struct Window *NiKWindow;
struct System *Servermem;

void startupNiKom(void) {
  struct MsgPort *port;

  Forbid();
  port = (struct MsgPort *)FindPort("NiKomPort");
  if(port == NULL) {
    NiKPort = (struct MsgPort *)CreatePort("NiKomPort",0);
  }
  Permit();
  if(port != NULL) {
    printf("NiKServer is already running.\n");
    exit(10);
  }
  if(NiKPort == NULL) {
    printf("Can't create message port 'NiKomPort'\n");
    exit(10);
  }
  openLibrariesAndPorts();
  setupServermem();
  initLanguages();
  
  readSysInfo();
  readTextInfo();
  readGroupData();
  readConferenceData();
  readConferenceTexts();
  readUserData();
  readLastLogins();
  readFileAreas();
  readFileAreaFiles();
  GetServerversion();
  scanFidoConferences();
  InitLegacyConversionData();

  Servermem->cfg = ReadAllConfigs();
  if(Servermem->cfg == NULL) {
    cleanup(EXIT_ERROR, "Error reading configs");
  }

  initNodes();
  openWindow();
}

void RegisterARexxFunctionHost(int add) {
  struct RexxMsg *mess;
  struct MsgPort *rexxmastport;
  if(!(mess=(struct RexxMsg *)AllocMem(sizeof(struct RexxMsg),
                                       MEMF_CLEAR | MEMF_PUBLIC))) {
    cleanup(EXIT_ERROR, "Out of memory.");
  }
  mess->rm_Node.mn_Node.ln_Type = NT_MESSAGE;
  mess->rm_Node.mn_Length = sizeof(struct RexxMsg);
  mess->rm_Node.mn_ReplyPort=rexxport;
  mess->rm_Action = add ? RXADDFH : RXREMLIB;
  mess->rm_Args[0] = "NIKOMREXXHOST";
  mess->rm_Args[1] = 0L;
  Forbid();
  rexxmastport = (struct MsgPort *)FindPort("REXX");
  if(rexxmastport) {
    PutMsg((struct MsgPort *)rexxmastport,(struct Message *)mess);
  }
  Permit();
  if(rexxmastport == NULL) {
    FreeMem(mess, sizeof(struct RexxMsg));
    if(add) {
      cleanup(EXIT_ERROR, "Can't find port 'REXX' (ARexx master server)");
    }
    return;
  }
  WaitPort(rexxport);
  GetMsg(rexxport);
  FreeMem(mess, sizeof(struct RexxMsg));
}

void openLibrariesAndPorts(void) {
  if(!(IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library",
                                                           MINOSVERSION))) {
    cleanup(EXIT_ERROR,"Can't open intuition.library.");
  }
  if(!(RexxSysBase = (struct RsxLib *)OpenLibrary("rexxsyslib.library",0L))) {
    cleanup(EXIT_ERROR,"Can't open rexxsyslib.library.");
  }
  if(!(UtilityBase = OpenLibrary("utility.library", MINOSVERSION))) {
    cleanup(EXIT_ERROR,"Can't open utility.library.");
  }
  if(!(NiKomBase = OpenLibrary("nikom.library", NIKLIBVERSION))) {
    cleanup(EXIT_ERROR, "Can't open nikom.library v" NIKLIBVERSIONSTR " or later.");
  }
  if(!(permitport = (struct MsgPort *)CreatePort("NiKomPermit",0))) {
    cleanup(EXIT_ERROR,"Can't open port 'NiKomPermit'");
  }
  if(!(nodereplyport = CreateMsgPort())) {
    cleanup(EXIT_ERROR,"Can't open node reply message port.");
  }

  if(!(rexxport=(struct MsgPort *)CreatePort("NIKOMREXXHOST",0))) {
    cleanup(EXIT_ERROR, "Could not open ARexx function host port.");
  }
  RegisterARexxFunctionHost(TRUE);
}

void setupServermem(void) {
  int i;
  if(!(Servermem = (struct System *)AllocMem(sizeof(struct System),
                                             MEMF_PUBLIC | MEMF_CLEAR))) {
    cleanup(EXIT_ERROR,"Can't allocate NiKServer shared memory.");
  }
  NewList((struct List *)&Servermem->user_list);
  NewList((struct List *)&Servermem->grupp_list);
  NewList((struct List *)&Servermem->shell_list);
  NewList((struct List *)&Servermem->mot_list);
  for(i = 0; i < MAXAREA; i++) {
    NewList((struct List *)&Servermem->areor[i].ar_list);
  }
  for(i=0; i < NIKSEM_NOOF; i++) {
    InitSemaphore(&Servermem->semaphores[i]);
  }
  InitServermem(Servermem); /* Kör igång nikom.library */
}

#define SYSINFO_FILEPATH "NiKom:DatoCfg/Sysinfo.dat"

void readSysInfo() {
  BPTR fh;

  if(!(fh=Open(SYSINFO_FILEPATH, MODE_OLDFILE))) {
    cleanup(EXIT_ERROR,"Couldn't open " SYSINFO_FILEPATH "\n");
  }
  if(Read(fh,(void *)&Servermem->info,sizeof(struct SysInfo))
     != sizeof(struct SysInfo)) {
    Close(fh);
    cleanup(EXIT_ERROR, "Couldn't read " SYSINFO_FILEPATH "\n");
  }
  printf("Sysinfo.dat inläst\n");
  Close(fh);
}

void readTextInfo(void) {  
  struct FileInfoBlock *fib;
  BPTR lock, fh;
  ULONG high=0, low=-1L, nr;
  char filename[40];
  struct Header readhead;

  if(!(fib=(struct FileInfoBlock *)AllocMem(sizeof(struct FileInfoBlock),
                                            MEMF_CLEAR))) {
    cleanup(EXIT_ERROR, "Couldn't allocate a FileInfoBlock");
  }
  if(!(lock=Lock("NiKom:Moten",ACCESS_READ))) {
    FreeMem(fib,sizeof(struct FileInfoBlock));
    cleanup(EXIT_ERROR, "Couldn't lock NiKom:Moten.");
  }
  if(!Examine(lock,fib)) {
    FreeMem(fib,sizeof(struct FileInfoBlock));
    UnLock(lock);
    cleanup(EXIT_ERROR, "Couldn't Examine() NiKom:Moten.");
  }
  while(ExNext(lock, fib)) {
    if(strncmp(fib->fib_FileName, "Head", 4)) {
      continue;
    }
    nr = atoi(&fib->fib_FileName[4]);
    if(nr > high) {
      high = nr;
    }
    if(nr < low) {
      low=nr;
    }
  }
  UnLock(lock);
  FreeMem(fib,sizeof(struct FileInfoBlock));
  
  if(low == -1L) {
    printf("No Head.dat files found. Assuming system has no texts yet.\n");
    Servermem->info.hightext = -1L;
    Servermem->info.lowtext = 0L;
    return;
  }

  sprintf(filename, "NiKom:Moten/Head%ld.dat", low);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    printf("Can't open %s.\n", filename);
    cleanup(EXIT_ERROR, "Couldn't get lowest textnumber.");
  }
  if(Seek(fh,0,OFFSET_BEGINNING) == -1) {
    Close(fh);
    cleanup(EXIT_ERROR, "Couldn't Seek() in Head.dat (1)");
  }
  if(Read(fh, &readhead, sizeof(struct Header)) != sizeof(struct Header)) {
    Close(fh);
    cleanup(EXIT_ERROR, "Couldn't read Head.dat (1)");
  }
  Servermem->info.lowtext = readhead.nummer;
  Close(fh);
  
  sprintf(filename,"NiKom:Moten/Head%ld.dat", high);
  if(!(fh = Open(filename,MODE_OLDFILE))) {
    printf("Can't open %s.\n", filename);
    cleanup(EXIT_ERROR, "Couldn't get highest textnumber.");
  }
  if(Seek(fh, -sizeof(struct Header), OFFSET_END) == -1) {
    Close(fh);
    cleanup(EXIT_ERROR, "Couldn't Seek() in Head.dat (2)");
  }
  if(Read(fh, &readhead, sizeof(struct Header)) != sizeof(struct Header)) {
    Close(fh);
    cleanup(EXIT_ERROR, "Couldn't read Head.dat (2)");
  }
  Servermem->info.hightext = readhead.nummer;
  Close(fh);
  printf("Lowtext: %ld  Hightext: %ld\n",
         Servermem->info.lowtext, Servermem->info.hightext);
}

void readGroupData(void) {
  BPTR fh;
  int x = 0;
  struct UserGroup *userGroup;

  NewList((struct List *)&Servermem->grupp_list);
  if(!(fh=Open("NiKom:DatoCfg/Grupper.dat",MODE_OLDFILE))) {
    cleanup(EXIT_ERROR, "Couldn't open NiKom:DatoCfg/Grupper.dat.");
  }
  
  for(;;) {
    if(!(userGroup=(struct UserGroup *)AllocMem(sizeof(struct UserGroup),
                                                MEMF_CLEAR | MEMF_PUBLIC))) {
      cleanup(EXIT_ERROR, "Out of memory.");
    }
    SetIoErr(0L);
    if(!FRead(fh, userGroup, sizeof(struct UserGroup), 1)) {
      if(IoErr()) {
        FreeMem(userGroup, sizeof(struct UserGroup));
        Close(fh);
        cleanup(EXIT_ERROR, "Error reading Grupper.dat");
      } else {
        FreeMem(userGroup, sizeof(struct UserGroup));
        break;
      }
    }
    if(!userGroup->namn[0]) {
      x++;
      continue;
    }
    AddTail((struct List *)&Servermem->grupp_list, (struct Node *)userGroup);
    userGroup->nummer = x;
    x++;
  }
  Close(fh);
  printf("Read %d user groups.\n", x);
}

void readConferenceData(void) {
  BPTR fh;
  int x = 0, ret;
  struct Mote *newConf, *conf;

  if(!(fh = Open("NiKom:DatoCfg/Möten.dat", MODE_OLDFILE))) {
    cleanup(EXIT_ERROR, "Could not open NiKom:DatoCfg/Möten.dat");
  }
  for(;;) {
    if(!(newConf = (struct Mote *)AllocMem(sizeof(struct Mote),
                                           MEMF_CLEAR | MEMF_PUBLIC))) {
      cleanup(EXIT_ERROR, "Out of memory.");
    }
    ret = Read(fh, newConf, sizeof(struct Mote));
    if(ret == -1) {
      FreeMem(newConf, sizeof(struct Mote));
      Close(fh);
      cleanup(EXIT_ERROR, "Error reading Möten.dat");
    } else if(ret == 0) {
      FreeMem(newConf, sizeof(struct Mote));
      break;
    }
    if(!newConf->namn[0]) {
      x++;
      continue;
    }
    newConf->nummer = x;
    ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
      if(conf->sortpri > newConf->sortpri) {
        break;
      }
    }
    Insert((struct List *)&Servermem->mot_list, (struct Node *)newConf,
           (struct Node *)conf->mot_node.mln_Pred);
    x++;
  }
  Close(fh);
  printf("Read %d forums.\n", x);
}

void readConferenceTexts(void) {
  int numberOfTexts;
  BPTR file;

  numberOfTexts = Servermem->info.hightext - Servermem->info.lowtext + 1;
  Servermem->confTexts.arraySize = numberOfTexts + 1000;
  if(!(Servermem->confTexts.texts =
       AllocMem(Servermem->confTexts.arraySize * sizeof(short),
                MEMF_CLEAR | MEMF_PUBLIC))) {
    cleanup(EXIT_ERROR, "Can't allocate ConferenceTexts array");
  }

  if(maybeConvertConferenceTextData(numberOfTexts)) {
    return;
  }

  printf("Reading ConferenceTexts.dat\n");
  if(!(file = Open("NiKom:DatoCfg/ConferenceTexts.dat",MODE_OLDFILE))) {
    cleanup(EXIT_ERROR, "Can't open ConferenceTexts.dat");
  }
  readIntoConfTextsArray(0, 0, numberOfTexts, file);
  Close(file);
}

#define OLD_MAX_TEXTS 32768

int maybeConvertConferenceTextData(int numberOfTexts) {
  BPTR file;
  int textsToRead, lowtextArrayPos, hightextArrayPos;

  if(!(file = Open("NiKom:DatoCfg/Textmot.dat",MODE_OLDFILE))) {
    return 0;
  }
  printf("Converting old Textmot.dat into ConferenceTexts.dat.\n");
  printf("  Number of texts to convert: %d\n", numberOfTexts);

  lowtextArrayPos = Servermem->info.lowtext % OLD_MAX_TEXTS;
  hightextArrayPos = Servermem->info.hightext % OLD_MAX_TEXTS;

  if(lowtextArrayPos > hightextArrayPos) {
    readIntoConfTextsArray(OLD_MAX_TEXTS - lowtextArrayPos,
                           0, hightextArrayPos + 1, file);
    textsToRead = OLD_MAX_TEXTS - lowtextArrayPos;
  } else {
    textsToRead = hightextArrayPos - lowtextArrayPos + 1;
  }
  readIntoConfTextsArray(0, lowtextArrayPos, textsToRead, file);
  Close(file);

  printf("  Textmot.dat read, writing ConferenceTexts.dat.\n");
  if(!WriteConferenceTexts()) {
    cleanup(EXIT_ERROR, "Couldn't write ConferenceTexts.dat.");
  }
  printf("  ConferenceTexts.dat written, deleting Textmot.dat.\n");
  if(!DeleteFile("NiKom:DatoCfg/Textmot.dat")) {
    cleanup(EXIT_ERROR, "Couldn't delete Textmot.dat.");
  }
  printf("  Conversion finished.\n");
  return 1;
}

void readIntoConfTextsArray(int arrayIndex, int fileIndex, int textsToRead,
                            BPTR file) {
  printf("  Reading from position %d.\n", fileIndex);
  if(Seek(file, fileIndex * sizeof(short), OFFSET_BEGINNING) == -1) {
    Close(file);
    cleanup(EXIT_ERROR, "Couldn't seek in file.");
  }
  printf("  Reading %d texts into index %d.\n", textsToRead, arrayIndex);
  if(FRead(file, &Servermem->confTexts.texts[arrayIndex], sizeof(short), textsToRead)
     != textsToRead) {
    Close(file);
    cleanup(EXIT_ERROR, "Couldn't read from file.");
  }
}

int readSingleUser(int userId) {
  BPTR fh;
  struct User tempuser;
  struct ShortUser *shortUser,*newShortUser;
  char filename[100];

  sprintf(filename, "NiKom:Users/%d/%d/Data", userId / 100, userId);
  if(!(fh = Open(filename, MODE_OLDFILE))) {
    printf("Could not open %s\n", filename);
    return 0;
  }
  if(Read(fh, &tempuser, sizeof(struct User)) == -1) {
    printf("Error reading %s\n", filename);
    Close(fh);
    return 0;
  }
  Close(fh);
  if(!(newShortUser = (struct ShortUser *)AllocMem(sizeof(struct ShortUser),
                                                   MEMF_CLEAR | MEMF_PUBLIC))) {
    cleanup(EXIT_ERROR,"Out of memory.");
  }
  ITER_EL(shortUser, Servermem->user_list, user_node, struct ShortUser *) {
    if(shortUser->nummer > userId) {
      break;
    }
  }
  Insert((struct List *)&Servermem->user_list, (struct Node *)newShortUser,
         (struct Node *)shortUser->user_node.mln_Pred);
  newShortUser->nummer = userId;
  newShortUser->status = tempuser.status;
  newShortUser->grupper = tempuser.grupper;
  strcpy(newShortUser->namn, tempuser.namn);
  return 1;
}

int scanUserDir(char *dirname) {
  BPTR lock;
  int goon;
  char __aligned buffer[2048];
  struct ExAllControl *ec;
  struct ExAllData *ed;
  char path[100];

  printf("  Reading user directory %s...\n", dirname);
  if(!(ec = (struct ExAllControl *)AllocDosObject(DOS_EXALLCONTROL, NULL))) {
    cleanup(EXIT_ERROR,"Out of memory.");
  }
  sprintf(path, "NiKom:Users/%s", dirname);
  if(!(lock = Lock(path, ACCESS_READ))) {
    FreeDosObject(DOS_EXALLCONTROL, ec);
    cleanup(EXIT_ERROR,"Could not Lock() NiKom:Users/x");
  }
  ec->eac_LastKey = 0L;
  ec->eac_MatchString = NULL;
  ec->eac_MatchFunc = NULL;
  goon = ExAll(lock, (struct ExAllData *)buffer, 2048, ED_NAME, ec);
  ed = (struct ExAllData *)buffer;
  for(;;) {
    if(!ec->eac_Entries) {
      if(!goon) {
        break;
      }
      goon = ExAll(lock, (struct ExAllData *)buffer, 2048, ED_NAME, ec);
      ed = (struct ExAllData *)buffer;
      continue;
    }
    if(!readSingleUser(atoi(ed->ed_Name))) {
      UnLock(lock);
      FreeDosObject(DOS_EXALLCONTROL, ec);
      return 0;
    }
    ed = ed->ed_Next;
    if(!ed) {
      if(!goon) {
        break;
      }
      goon = ExAll(lock, (struct ExAllData *)buffer, 2048, ED_NAME, ec);
      ed = (struct ExAllData *)buffer;
    }
  }
  UnLock(lock);
  FreeDosObject(DOS_EXALLCONTROL, ec);
  return 1;
}

void readUserData(void) {
  BPTR lock;
  int goon;
  char __aligned buffer[2048];
  struct ExAllControl *ec;
  struct ExAllData *ed;

  printf("Reading user data...\n");

  if(!(ec = (struct ExAllControl *)AllocDosObject(DOS_EXALLCONTROL, NULL))) {
    cleanup(EXIT_ERROR, "Out of memory.");
  }
  if(!(lock = Lock("NiKom:Users", ACCESS_READ))) {
    FreeDosObject(DOS_EXALLCONTROL, ec);
    cleanup(EXIT_ERROR,"Could not Lock() NiKom:Users\n");
  }
  ec->eac_LastKey = 0L;
  ec->eac_MatchString = NULL;
  ec->eac_MatchFunc = NULL;
  goon = ExAll(lock, (struct ExAllData *)buffer, 2048, ED_NAME, ec);
  ed = (struct ExAllData *)buffer;
  for(;;) {
    if(!ec->eac_Entries) {
      if(!goon) {
        break;
      }
      goon = ExAll(lock, (struct ExAllData *)buffer, 2048, ED_NAME, ec);
      ed = (struct ExAllData *)buffer;
      continue;
    }
    if(!scanUserDir(ed->ed_Name)) {
      UnLock(lock);
      FreeDosObject(DOS_EXALLCONTROL,ec);
      cleanup(EXIT_ERROR, "Could not read users.");
    }
    ed = ed->ed_Next;
    if(!ed) {
      if(!goon) {
        break;
      }
      goon = ExAll(lock,(struct ExAllData *)buffer,2048,ED_NAME,ec);
      ed = (struct ExAllData *)buffer;
    }
  }
  UnLock(lock);
  FreeDosObject(DOS_EXALLCONTROL,ec);

  printf("Read users (Highest id: %ld)\n",
         ((struct ShortUser *)Servermem->user_list.mlh_TailPred)->nummer);
  printf("User #0 : %s\n",((struct ShortUser *)Servermem->user_list.mlh_Head)->namn);
}

void readLastLogins(void) {
  BPTR fh;
  if(!(fh = Open("NiKom:DatoCfg/Senaste.dat", MODE_OLDFILE))) {
    cleanup(EXIT_ERROR, "Could not read Senaste.dat");
  }
  if(!Read(fh, Servermem->senaste, sizeof(struct Inloggning) * MAXSENASTE)) {
    Close(fh);
    cleanup(EXIT_ERROR, "Error reading last login data.");
  }
  Close(fh);
  printf("Read last login data.\n");
}

void readFileAreas(void) {
  BPTR fh;
  int i;
  if(!(fh = Open("NiKom:DatoCfg/Areor.dat", MODE_OLDFILE))) {
    cleanup(EXIT_ERROR,"Could not open NiKom:DatoCfg/Areor.dat");
  }
  SetIoErr(0L);
  Servermem->info.areor = FRead(fh, (void *)Servermem->areor,
                                sizeof(struct Area), MAXAREA);
  if(IoErr()) {
    cleanup(EXIT_ERROR, "Error reading Areor.dat");
  }
  Close(fh);
  for(i = 0; i < MAXAREA; i++) {
    NewList((struct List *)&Servermem->areor[i].ar_list);
  }
  printf("Read %d file areas.\n", Servermem->info.areor);
}

void readFileAreaFiles(void) {
  int i, readlen, index, cnt;
  BPTR fh;
  struct Fil *newFile, *searchFile;
  char path[110];

  for(i = 0; i < Servermem->info.areor; i++) {
    if(!Servermem->areor[i].namn[0]) {
      continue;
    }
    printf("  Reading file area %s...", Servermem->areor[i].namn);
    sprintf(path, "NiKom:DatoCfg/Areor/%d.dat", i);
    if(!(fh = Open(path, MODE_OLDFILE))) {
      cleanup(EXIT_ERROR, "Could not open Areor/x.dat");
    }
    index = 0;
    cnt = 0;
    for(;;) {
      if(!(newFile = (struct Fil *)AllocMem(sizeof(struct Fil),
                                            MEMF_CLEAR | MEMF_PUBLIC))) {
        Close(fh);
        cleanup(EXIT_ERROR, "Out of memory.");
      }
      if((readlen = Read(fh, newFile, sizeof(struct DiskFil)))
         != sizeof(struct DiskFil)) {
        FreeMem(newFile, sizeof(struct DiskFil));
        Close(fh);
        if(readlen != 0) {
          cleanup(EXIT_ERROR, "Error reading Areor/x.dat");
        }
        break;
      }
      newFile->index = index++;
      if(newFile->namn[0] == 0) {
        // File is deleted
        continue;
      }
      ITER_EL(searchFile, Servermem->areor[i].ar_list, f_node, struct Fil *) {
        if(searchFile->tid > newFile->tid) {
          break;
        }
      }
      Insert((struct List *)&Servermem->areor[i].ar_list, (struct Node *)newFile,
             (struct Node *)searchFile->f_node.mln_Pred);
      cnt++;
    }
    printf(" %d files.\n", cnt);
  }
  printf("Read file area files.\n");
}

void scanFidoConferences(void) {
  struct Mote *conf;
  printf("Scanning Fido forums...\n");
  ITER_EL(conf, Servermem->mot_list, mot_node, struct Mote *) {
    if(conf->type != MOTE_FIDO) {
      continue;
    }
    printf("   %s...\n", conf->namn);
    ReScanFidoConf(conf, 0);
  }
}

void initNodes(void) {
  int i;
  for(i = 0; i < MAXNOD; i++) {
    Servermem->nodtyp[i] = 0;
    Servermem->inloggad[i] = -1;
    Servermem->action[i] = 0;
    Servermem->maxinactivetime[i] = 5;
    Servermem->watchserial[i] = 1;
  }
}

void initLanguages(void) {
  Servermem->languages[0] = "english";
  Servermem->languages[1] = "svenska";
}

void openWindow(void) {
  int windowHeight;
  struct Screen *lockscreen;

  strcpy(NiKomReleaseStr, NIKRELEASE " (" __DATE__ " " __TIME__ ")");
  sprintf(windowTitle, "NiKom %s,  0 noder aktiva", NiKomReleaseStr);
  if(!(lockscreen = LockPubScreen(pubscreen[0] == '-' ? NULL : pubscreen))) {
    cleanup(EXIT_ERROR,"Kunde inte låsa angiven Public Screen\n");
  }
  windowHeight = lockscreen->WBorTop + lockscreen->Font->ta_YSize + 1;
  NiKWindow = (struct Window *)OpenWindowTags(NULL,
                                              WA_Left, xpos,
                                              WA_Top, ypos,
                                              WA_Width, 500,
                                              WA_Height, windowHeight,
                                              WA_IDCMP, IDCMP_CLOSEWINDOW,
                                              WA_Title, windowTitle,
                                              WA_SizeGadget, FALSE,
                                              WA_DragBar, TRUE,
                                              WA_DepthGadget, TRUE,
                                              WA_CloseGadget, TRUE,
                                              WA_NoCareRefresh, TRUE,
                                              WA_ScreenTitle, "NiKom Server",
                                              WA_AutoAdjust, TRUE,
                                              WA_PubScreen, lockscreen);
  UnlockPubScreen(NULL, lockscreen);
  if(NiKWindow == NULL) {
    cleanup(EXIT_ERROR, "Could not open window.");
  }
}
