#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <sys/stat.h>

#include "FCrypt.h"

#define MAXMOTE			2048
#define MAXVOTE			32

struct User {
   long tot_tid,forst_in,senast_in,read,skrivit,flaggor,former_textpek,brevpek,
      grupper,defarea,downloadbytes,chrset,uploadbytes,reserv5,upload,download,
      loggin,shell;
   char namn[41],gata[41],postadress[41],land[41],telefon[21],
      annan_info[61],losen[16],status,rader,protokoll,
      prompt[6],motratt[MAXMOTE/8],motmed[MAXMOTE/8],vote[MAXVOTE];
};

void encryptString(char *str) {
  char salt[3];
  generateSalt(salt, 2);
  printf("'%s' --> '%s'\n", str, crypt(str, salt));
}

void convertUser(int userId) {
  char filename[40], salt[3], *encryptedPwd;
  struct stat statbuf;
  FILE *fp;
  struct User userbuf;

  sprintf(filename, "NiKom:Users/%d/%d/Data", userId / 100, userId);
  if(stat(filename, &statbuf) != 0) {
    printf("Can't find data for user %d\n", userId);
    return;
  }
  if(statbuf.st_size != sizeof(struct User)) {
    printf("Size of user's data file is %d, was expecting %lu.\n",
           statbuf.st_size, (unsigned long)sizeof(struct User));
    printf("Are you running the right version of CryptPasswords?\n");
    return;
  }
  if(!(fp = fopen(filename, "rb+"))) {
    printf("Can't open %s\n", filename);
    return;
  }
  if(fread(&userbuf, sizeof(struct User), 1, fp) != 1) {
    printf("Could not read the user's data file.\n");
    fclose(fp);
    return;
  }

  generateSalt(salt, 2);
  encryptedPwd = crypt(userbuf.losen, salt);
  strncpy(userbuf.losen, encryptedPwd, 16);

  rewind(fp);
  if(fwrite(&userbuf, sizeof(struct User), 1, fp) != 1) {
    printf("Could not write the user's data file.\n");
  } else {
    printf("Password for user %d is encrypted.\n", userId);
  }
  fclose(fp);
}

void printUsage(int exitCode) {
    printf("Usage: CryptPasswords -s <string> | -u <userid> | -a\n\n");
    printf("-s <string> : Encrypt the given string.\n");
    printf("-u <userid> : Encrypt the password for the given user.\n");
    printf("-a          : Encrypt the password for all users.\n");
    exit(exitCode);
}

void main(int argc, char *argv[]) {
  if(argc < 2 || argv[1][0] != '-') {
    printUsage(0);
  }
  switch(argv[1][1]) {
  case 's':
    if(argc != 3) {
      printf("Invalid number of arguments for -s\n\n");
      printUsage(10);
    }
    encryptString(argv[2]);
    break;
  case 'u':
    if(argc != 3) {
      printf("Invalid number of arguments for -u\n\n");
      printUsage(10);
    }
    if(!isdigit(argv[2][0])) {
      printf("Invalid userid\n");
      exit(10);
    }
    convertUser(atoi(argv[2]));
    break;
  case 'a':
    if(argc != 2) {
      printf("No argument allowed for -a\n\n");
      printUsage(10);
    }
    printf("-a option is not implemented yet..\n");
    break;
  default:
    printf("Invalid option.\n\n");
    printUsage(10);
  }
}
