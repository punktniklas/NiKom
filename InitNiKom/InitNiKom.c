#include <exec/types.h>
#include <rexx/storage.h>
#include <utility/tagitem.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <dos.h>
#include <dos/dos.h>
#include <dos/exall.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "NiKomStr.h"

int deleteDir(char *dirname);

struct Library *NiKomBase;

/* A minimal user with name "Sysop" and password "sysop". */
struct User userData = {
	0L,0L,0L,0L,0L,0L,0L,0L,0L,0L,0L,0L,0L,0L,0L,0L,0L,0L,
	"Sysop","","","","","","hX62xN2A/dI0Q",
	100,25,0,"-->","","",""
};
struct SysInfo sysInfo;
struct UnreadTexts unreadTexts;

void createDirOrDie(char *dirname) {
  BPTR lock;
  struct FileInfoBlock __aligned fib;
  
  if(lock = Lock(dirname, ACCESS_READ)) {
    if(!Examine(lock, &fib)) {
      printf("Could not examine %s\n", dirname);
      UnLock(lock);
      exit(10);
    }
    UnLock(lock);
    if(fib.fib_DirEntryType <= 0) {
      printf("Can't create dir %s, there is a file with the same name.\n", dirname);
      exit(10);
    }
    return;
  }
  if(!(lock=CreateDir(dirname))) {
    printf("Couldn't create directory %s\n", dirname);
    exit(10);
  }
  UnLock(lock);
  printf("Created directory %s\n", dirname);
}

void createFile(char *filename, void *data, int size) {
  FILE *fp;
  if(fp=fopen(filename, "w")) {
    if(size > 0) {
      fwrite(data, size, 1, fp);
    }
    fclose(fp);
  } else {
    printf("Could not write %s\n", filename);
    exit(10);
  }
  printf("Created file %s\n", filename);
}

void createEmptyFile(char *filename) {
  createFile(filename, NULL, 0);
}

void initSysop(void) {
  long tid;

  createDirOrDie("NiKom:Users");
  createDirOrDie("NiKom:Users/0");
  createDirOrDie("NiKom:Users/0/0");

  time(&tid);
  userData.forst_in=tid;
  userData.senast_in=tid;
  createFile("NiKom:Users/0/0/Data", &userData, sizeof(struct User));

  unreadTexts.bitmapStartText = 0;
  memset(unreadTexts.bitmap, 0xff, UNREADTEXTS_BITMAPSIZE/8);
  memset(unreadTexts.lowestPossibleUnreadText, 0, MAXMOTE * sizeof(long));
  createFile("NiKom:Users/0/0/UnreadTexts", &unreadTexts, sizeof(struct UnreadTexts));

  createFile("Nikom:Users/0/0/.firstletter", "0", 1);
  createFile("Nikom:Users/0/0/.nextletter", "0", 1);
}

void initSysInfo(void) {
  memset(&sysInfo, 0, sizeof(struct SysInfo));
  createFile("NiKom:DatoCfg/Sysinfo.dat", &sysInfo, sizeof(struct SysInfo));
}

void initFiles(void) {
  initSysop();
  createDirOrDie("NiKom:DatoCfg");
  createDirOrDie("NiKom:Moten");
  initSysInfo();
  createEmptyFile("NiKom:DatoCfg/Möten.dat");
  createEmptyFile("NiKom:DatoCfg/ConferenceTexts.dat");
  createEmptyFile("NiKom:DatoCfg/Areor.dat");
  createEmptyFile("NiKom:DatoCfg/Grupper.dat");
}

int deleteFile(char *filename) {
  BPTR lock;

  if(!(lock = Lock(filename, ACCESS_READ))) {
    // Can't get lock, assume it doesn't exist.
    return 1;
  }
  UnLock(lock);
  if(!DeleteFile(filename)) {
    printf("Couldn't delete file/dir '%s'\n", filename);
    return 0;
  }
  printf("Deleted file/dir '%s'\n", filename);
  return 1;
}

void deleteFileOrDie(char *filename) {
  if(!deleteFile(filename)) {
    exit(10);
  }
}

int handleExAllResult(struct ExAllControl *ec, struct ExAllData *ed, char *dirname) {
  char filename[100];
  int success;
  if(ec->eac_Entries == 0) {
    return 1;
  }
  while(ed != NULL) {
    sprintf(filename, "%s/%s", dirname, ed->ed_Name);
    if(ed->ed_Type < 0) {
      success = deleteFile(filename);
    } else {
      success = deleteDir(filename);
    }
    if(!success) {
      return 0;
    }
    ed=ed->ed_Next;
  }
  return 1;
}

int deleteDir(char *dirname) {
  char __aligned buffer[2048];
  struct ExAllControl *ec;
  int moreFromExAll, success = TRUE;
  BPTR lock;

  printf("Deleting all files in directory %s\n", dirname);
  if(!(lock = Lock(dirname, ACCESS_READ))) {
    printf("Can't lock directory %s\n", dirname);
    return 0;
  }
  ec = (struct ExAllControl *) AllocDosObject(DOS_EXALLCONTROL, NULL);
  ec->eac_LastKey=0L;
  ec->eac_MatchString=NULL;
  ec->eac_MatchFunc=NULL;
  do {
    moreFromExAll = ExAll(lock, (struct ExAllData *)buffer, 2048, ED_TYPE, ec);
    if(!handleExAllResult(ec, (struct ExAllData *) buffer, dirname)) {
      success = FALSE;
      break;
    }
  } while(moreFromExAll);
  
  UnLock(lock);
  FreeDosObject(DOS_EXALLCONTROL,ec);
  if(success) {
    success = deleteFile(dirname);
  }
  return success;
}

void deleteDirOrDie(char *dirname) {
  if(!deleteDir(dirname)) {
    exit(10);
  }
}

void clearFiles(void) {
  deleteDirOrDie("NiKom:Users");
  deleteDirOrDie("NiKom:Moten");
  deleteFileOrDie("NiKom:DatoCfg/Sysinfo.dat");
  deleteFileOrDie("NiKom:DatoCfg/Möten.dat");
  deleteFileOrDie("NiKom:DatoCfg/ConferenceTexts.dat");
  deleteFileOrDie("NiKom:DatoCfg/Areor.dat");
  deleteFileOrDie("NiKom:DatoCfg/Grupper.dat");
}

int main(int argc, char **argv) {
  char input[20];
  int count=0;

  printf("The following files/dirs will be reset/deleted:\n");
  printf(" Users\n");
  printf(" Moten\n");
  printf(" DatoCfg/Sysinfo.dat\n");
  printf(" DatoCfg/Möten.dat\n");
  printf(" DatoCfg/ConferenceTexts.dat\n");
  printf(" DatoCfg/Areor.dat\n");
  printf(" DatoCfg/Grupper.dat\n\n");
  printf("Is this ok? (y/N) ");
  while((input[count++] = getchar()) != '\n');
  if(input[0] != 'y' && input[0] != 'Y') {
    printf("Nope!\n");
    return 0;
  }

  printf("Yep!\n\n");
  printf("Creating files necessary for NiKom to start up, please wait...\n");
  fflush(stdout);
  clearFiles();
  initFiles();
  printf("Done!\n");

  return 0;
}

