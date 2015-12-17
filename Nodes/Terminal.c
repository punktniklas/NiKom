#include <exec/types.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "NiKomstr.h"
#include "NiKomFuncs.h"
#include "NiKomLib.h"
#include "Logging.h"
#include "Terminal.h"
#include "BasicIO.h"

extern int nodnr, rxlinecount;
extern char outbuffer[];
extern struct System *Servermem;

char inmat[1024];     /* Används av getstring() */
int radcnt = 0;

char commandhistory[10][1025];

int checkvalue(char *buffer);
int handle1bChar(void);
int handleAnsiSequence(void);
int handleShiftedAnsiSequence(void);
int handleCsi3(void);

void sendfile(char *filename) {
  FILE *fp;
  char buff[1025];
  if(!(fp=fopen(filename, "r"))) {
    LogEvent(SYSTEM_LOG, ERROR, "Can't open file %s for reading.", filename);
    DisplayInternalError();
    return;
  }
  for(;;) {
    if(!(fgets(buff, 1023, fp))) {
      break;
    }
    if(SendString("%s\r", buff)) {
      break;
    }
  }
  fclose(fp);
}

int GetChar(void) {
  unsigned char ch;
  int ret;

  for(;;) {
    ch = gettekn();

    if(ImmediateLogout()) {
      return GETCHAR_LOGOUT;
    }

    switch(ch) {
    case '\r': case '\n':
      return GETCHAR_RETURN;
    case 1: // Ctrl-A
      return GETCHAR_SOL;
    case 5: // Ctrl-E
      return GETCHAR_EOL;
    case '\b':
      return GETCHAR_BACKSPACE;
    case 4: // Ctrl-D
      return GETCHAR_DELETE;
    case 127: // Delete
      return Servermem->inne[nodnr].flaggor & ASCII_7E_IS_DELETE
          || Servermem->nodtyp[nodnr] == NODCON
        ? GETCHAR_DELETE : GETCHAR_BACKSPACE;
    case 24: // Ctrl-X
      return GETCHAR_DELETELINE;
    case 0x9b:
      ret = handleAnsiSequence();
      if(ret != 0) {
        return ret;
      }
      break;
    case 0x1b:
      ret = handle1bChar();
      if(ret != 0) {
        return ret;
      }
      break;
    default:
      return ch;
    }
  }
}

int handle1bChar(void) {
  unsigned char ch;

  ch = gettekn();
  if(ImmediateLogout()) {
    return GETCHAR_LOGOUT;
  }
  if(ch == '[' || ch == 'Ä') {
    return handleAnsiSequence();
  }
  return 0;
}

/*
 * Handles characters after CSI (1b5b), i.e. ANSI escape sequences
 */
int handleAnsiSequence(void) {
  unsigned char ch;

  ch = gettekn();
  if(ImmediateLogout()) {
    return GETCHAR_LOGOUT;
  }

  switch(ch) {
  case 0x33:
    return handleCsi3();
  case 0x41: // Arrow up
    return GETCHAR_UP;
  case 0x42: //Arrow down
    return GETCHAR_DOWN;
  case 0x43: // Arrow right
    return GETCHAR_RIGHT;
  case 0x44: // Arrow left
    return GETCHAR_LEFT;
  case 0x20: // Shift-<something>
    return handleShiftedAnsiSequence();
  default:
    return 0;
  }
}

/*
 * Handles the CSI (0x1b5b) follwed by '3' (0x33)
 */
int handleCsi3(void) {
  unsigned char ch;

  ch = gettekn();
  if(ImmediateLogout()) {
    return GETCHAR_LOGOUT;
  }

  switch(ch) {
  case 0x7e:
    return GETCHAR_DELETE;
  default:
    return 0;
  }
}

int handleShiftedAnsiSequence(void) {
  unsigned char ch;

  ch = gettekn();
  if(ImmediateLogout()) {
    return GETCHAR_LOGOUT;
  }
  
  switch(ch) {
  case 0x40: // Shift Arrow Right
    return GETCHAR_EOL;
  case 0x41: // Shift Arrow Left
    return GETCHAR_SOL;
  default:
    return 0;
  }
}

// TODO: Phase out the use of this function in favor of Get(Secret)String
int getstring(int echo, int maxchrs, char *defaultStr) {
  return GetStringX(echo, maxchrs, defaultStr, NULL, NULL, NULL);
}

int GetString(int maxchrs, char *defaultStr) {
  return GetStringX(EKO, maxchrs, defaultStr, NULL, NULL, NULL);
}

int IsPrintableCharacter(unsigned char c) {
  return (c > 31 && c < 127) || (c > 159 && c <= 255);
}

int digitCharactersAccepted(unsigned char c) {
  return (c >= '0' && c <= '9') || c == '-';
}

struct GetNumberData {
  int minvalue, maxvalue;
};  

int numberStringAccepted(char *str, void *data) {
  struct GetNumberData *getNumberData = (struct GetNumberData *) data;
  int intvalue;
  char tmpstr[12];

  intvalue = atoi(str);
  sprintf(tmpstr, "%d", intvalue);
  if(strcmp(str, tmpstr) != 0) {
    return 0;
  }
  return intvalue >= getNumberData->minvalue && intvalue <= getNumberData->maxvalue;
}

int GetNumber(int minvalue, int maxvalue, char *defaultStr) {
  char minstr[12], maxstr[12];
  int minlen, maxlen;
  struct GetNumberData getNumberData;

  sprintf(minstr, "%d", minvalue);
  sprintf(maxstr, "%d", maxvalue);
  minlen = strlen(minstr);
  maxlen = strlen(maxstr);
  getNumberData.minvalue = minvalue;
  getNumberData.maxvalue = maxvalue;
  if(GetStringX(EKO, minlen > maxlen ? minlen : maxlen, defaultStr,
                digitCharactersAccepted, numberStringAccepted,
                &getNumberData)) {
    return 0;
  }
  return atoi(inmat);
}

int GetSecretString(int maxchrs, char *defaultStr) {
  return GetStringX(Servermem->inne[nodnr].flaggor & STAREKOFLAG ? STAREKO : EJEKO,
                    maxchrs, defaultStr, NULL, NULL, NULL);
}

int GetStringX(int echo, int maxchrs, char *defaultStr,
               int (*isCharacterAccepted)(unsigned char),
               int (*isStringAccepted)(char *, void *),
               void *customData) {
  int size = 0, pos=0, i, modified = FALSE, ch;
  static int historycnt = -1;
  radcnt = 0;

  memset(inmat,0,1023);
  if(defaultStr != NULL) {
    size = pos = strlen(defaultStr);
    SendStringNoBrk(defaultStr);
    strcpy(inmat, defaultStr);
  }
  if(isCharacterAccepted == NULL) {
    isCharacterAccepted = &IsPrintableCharacter;
  }

  for(;;) {
    ch = GetChar();

    if(ch == GETCHAR_LOGOUT) {
      return 1;
    }
    if(ch == GETCHAR_RETURN) {
      inmat[size] = '\0';
      if(isStringAccepted == NULL || isStringAccepted(inmat, customData)) {
        break;
      }
    }
    else if(ch == GETCHAR_SOL) {
      if(pos == 0) {
        continue;
      }
      if(echo) {
        SendStringNoBrk("\x1b\x5b%d\x44", pos);
      }
      pos = 0;
    }
    else if(ch == GETCHAR_EOL) {
      if(pos == size) {
        continue;
      }
      if(echo) {
        SendStringNoBrk("\x1b\x5b%d\x43", size - pos);
      }
      pos = size;
    }
    else if(ch == GETCHAR_BACKSPACE) {
      if(pos == 0) {
        continue;
      }
      if(echo) {
        if(Servermem->inne[nodnr].flaggor & SEKVENSANSI) {
          SendStringNoBrk("\x1b\x5b\x44\x1b\x5b\x50",-1,0);
        }
        else {
          SendStringNoBrk("\b \b",-1,0);
        }
      }
      movmem(&inmat[pos], &inmat[pos - 1], size - pos);
      pos--;
      size--;
      modified = TRUE;
    }
    else if(ch == GETCHAR_DELETE) {
      if(pos == size) {
        continue;
      }
      if(echo) {
        SendStringNoBrk("\x1b\x5b\x50");
      }
      movmem(&inmat[pos + 1], &inmat[pos], size - pos);
      size--;
      modified = TRUE;
    }
    else if(ch == GETCHAR_DELETELINE) {
      if(echo) {
        if(pos > 0) {
          SendStringNoBrk("\x1b\x5b%d\x44\x1b\x5b\x4a", pos);
        } else {
          SendStringNoBrk("\x1b\x5b\x4a");
        }
      }
      memset(inmat, 0, 1023);
      pos = size = 0;
    }
    else if(ch == GETCHAR_LEFT) {
      if(pos == 0) {
        continue;
      }
      if(echo) {
        SendStringNoBrk("\x1b\x5b\x44",-1,0);
      }
      pos--;
    } else if(ch == GETCHAR_RIGHT) {
      if(pos == size) {
        continue;
      }
      if(echo) {
        SendStringNoBrk("\x1b\x5b\x43",-1,0);
      }
      pos++;
    } else if(ch == GETCHAR_UP) {
      if(!echo || historycnt >= 9 || commandhistory[historycnt + 1][0] == '\0') {
        continue;
      }
      historycnt++;
      strncpy(inmat, commandhistory[historycnt], maxchrs);
      inmat[maxchrs] = '\0';
      if(pos > 0) {
        SendStringNoBrk("\x1b\x5b%d\x44\x1b\x5b\x4a%s", pos, inmat);
      } else {
        SendStringNoBrk("\x1b\x5b\x4a%s", inmat);
      }
      pos = size = strlen(inmat);
      modified=FALSE;
    } else if(ch == GETCHAR_DOWN) {
      if(!echo) {
        continue;
      }
      if(historycnt == 0 || historycnt == -1) {
        historycnt = -1;
        if(echo == 1) {
          if(pos > 0) {
            SendStringNoBrk("\x1b\x5b%d\x44\x1b\x5b\x4a",pos);
          } else {
            SendStringNoBrk("\x1b\x5b\x4a");
          }
        }
        memset(inmat,0,1023);
        pos=size=0;
        continue;
      }
        
      if(historycnt == 0 || commandhistory[historycnt-1][0] == '\0') {
        continue;
      }
      
      historycnt--;
      strncpy(inmat, commandhistory[historycnt], maxchrs);
      inmat[maxchrs] = '\0';
      if(pos) {
        SendStringNoBrk("\x1b\x5b%d\x44\x1b\x5b\x4a%s", pos, inmat);
      } else {
        SendStringNoBrk("\x1b\x5b\x4a%s", inmat);
      }
      pos = size = strlen(inmat);
      modified=FALSE;
    }
    else if(isCharacterAccepted(ch)) {
      if(size >= maxchrs) {
        eka('\a');
        continue;
      }
      if(echo) {
        if(Servermem->inne[nodnr].flaggor & SEKVENSANSI) {
          SendStringNoBrk("\x1b\x5b\x31\x40");
        }
        if(echo != STAREKO) {
          if(ch == '+') {
            SendStringNoBrk(" \b");
          }
          eka(ch);
        } else {
          eka('*');
        }
      }
      movmem(&inmat[pos], &inmat[pos+1], size - pos);
      inmat[pos++] = ch;
      size++;
      modified = TRUE;
    }
  }
  inmat[size] = 0;
  if(echo == STAREKO) {
    if(pos != size) {
      SendStringNoBrk("\x1b\x5b%d\x43", size - pos);
    }

    for(i = 0; i < size; i++) {
      if(Servermem->inne[nodnr].flaggor & SEKVENSANSI) {
        SendStringNoBrk("\x1b\x5b\x44\x1b\x5b\x50");
      } else {
        SendStringNoBrk("\b \b");
      }
    }
  }
  eka('\r');

  if(inmat[0] && echo == 1 && modified) {
    for(i = 9; i > 0; i--) {
      strcpy(commandhistory[i], commandhistory[i-1]);
    }
    strncpy(commandhistory[0], inmat, 1023);
    historycnt=-1;
  }
  if(historycnt > -1) {
    historycnt--;
  }
  return 0;
}

int incantrader(void) {
  char aborted = FALSE;
  int ch;
  
  radcnt++;
  if(Servermem->inne[nodnr].rader
     && radcnt >= Servermem->inne[nodnr].rader
     && rxlinecount) {
    SendStringNoBrk("\r<RETURN>");
    for(;;) {
      ch = GetChar();
      if(ch == GETCHAR_LOGOUT || ch == 3) { // Ctrl-C
        aborted = TRUE;
        break;
      }
      if(ch == GETCHAR_RETURN) {
        break;
      }
    }
    radcnt=0;
    putstring("\r        \r",-1,0);
  }
  return aborted;
}

int conputtekn(char *pekare,int size)
{
	int aborted=FALSE,x;
	char *tmppek, buffer[1200], constring[1201];

	strncpy(constring,pekare,1199);
	constring[1199]=0;
	pekare = &constring[0];

	if(!(Servermem->inne[nodnr].flaggor & ANSICOLOURS)) StripAnsiSequences(pekare);

	if(size == -1 && !pekare[0]) return(aborted);

	tmppek = pekare;
	x = 0;
	while((x < size || size == -1) && !aborted)
	{
		if(size < 1 && tmppek[0] == 0)
		{
			if(tmppek != pekare)
			{
				if(sendtocon(pekare, size)) aborted = TRUE;
			}
			break;
		}
		if(size > 0 && x++ == size)
		{
			if(pekare != tmppek)
			{
				tmppek[1] = 0;
				if(sendtocon(pekare, -1)) aborted = TRUE;
			}
			break;
		}
		if(tmppek[0] == '\n')
		{
			buffer[0] = 0;
			if(tmppek != pekare)
			{
				tmppek[0] = 0;
				if(pekare[0] != NULL)
					strcpy(buffer, pekare);

				strcat(buffer, "\n");
				pekare = ++tmppek;
			}
			else
			{
				buffer[0] = '\n';
				buffer[1] = NULL;
				pekare = ++tmppek;
			}

			if(sendtocon(&buffer[0], -1)) aborted = TRUE;
			tmppek = pekare - 1;
		}
		tmppek++;
	}

	return(aborted);
}

/*
 * Sends a string to the user. The arguments are compatible with printf().
 * The first argument is a char pointer to a format string and it's
 * followed by zero or more arguments to resolve the placeholders in the
 * format string.
 *
 * Returns 0 if everything is normal or non zero if sending was interrupted.
 * Interruption could be e.g. because the user has pressed Ctrl-C or that
 * the carrier has been dropped.
 *
 * TODO: This should be the default function to use for any code that
 * wants to do normal output to the user. Over time calls to puttekn()
 * should be replaced with calls to SendString() and the global variable
 * outbuffer should be made internal to this function/module.
 * There will probably have to be added other versions of this function
 * like SendStringCon() and a function to replace putstring() (that
 * doesn't count lines and pause with a <RETURN> prompt). They should
 * all just be wrappers to one common code path to send strings.
 */
int SendString(char *fmt, ...) {
  va_list arglist;
  int ret;

  va_start(arglist, fmt);
  vsprintf(outbuffer, fmt, arglist);
  ret = puttekn(outbuffer, -1);
  va_end(arglist);
  return ret;
}

int SendStringNoBrk(char *fmt, ...) {
  va_list arglist;

  va_start(arglist, fmt);
  vsprintf(outbuffer, fmt, arglist);
  putstring(outbuffer, -1, 0);
  va_end(arglist);
  return 0;
}

void DisplayInternalError(void) {
  SendString("\r\n\n*** Internal error ***\r\n\n");
}

/*
 * Displays a yes/no question and waits for an answer. yesChar and
 * noChar must be lowercase (but can be 8-bit non-ASCII). Result
 * is returned in res. 1 if yesChar was selected, 0 otherwise.
 * Function return 0 normally or 1 if carrier is dropped.
 */
int GetYesOrNo(char *label, char yesChar, char noChar, char *yesStr, char *noStr,
               int yesIsDefault, int *res) {
  int ch;
  char *response;

  radcnt = 0;
  SendString("%s (%c/%c) ", label != NULL ? label : "",
             yesIsDefault ? yesChar - 32 : yesChar,
             yesIsDefault ? noChar : noChar - 32);
  for(;;) {
    ch = GetChar();
    if(ch == GETCHAR_LOGOUT) {
      return 1;
    }
    if(ch == GETCHAR_RETURN) {
      *res = yesIsDefault ? 1 : 0;
    } else if(ch == yesChar || ch == yesChar - 32) {
      *res = 1;
    } else if(ch == noChar || ch == noChar - 32) {
      *res = 0;
    } else {
      continue;
    }
    response = *res ? yesStr : noStr;
    if(response != NULL)  {
      SendString(response);
    }
    return 0;
  }
}

int EditString(char *label, char *str, int maxlen, int nonEmpty) {
  for(;;) {
    SendString("\r\n%s : ", label);
    if(GetString(maxlen, NULL)) {
      return 1;
    }
    if(inmat[0] == '\0' && nonEmpty) {
      continue;
    }
    strncpy(str, inmat, maxlen + 1);
    return 0;
  }
}

/*
 * Asks the user to edit the given string. str must be able to hold maxlen + 1
 * characters (for the trailing '\0' character).
 */
int MaybeEditString(char *label, char *str, int maxlen) {
  SendString("\r\n%s : (%s) ", label, str);
  if(GetString(maxlen, NULL)) {
    return 1;
  }
  if(inmat[0]) {
    strncpy(str, inmat, maxlen + 1);
  }
  return 0;
}

/*
 * Asks the user to input a password. If a password is entered and
 * re-entered successfully it is written to pwd. If an empty password
 * is given pwd is unchanged.
 */
int MaybeEditPassword(char *label1, char *label2, char *pwd, int maxlen) {
  char tmpStr[50];
  for(;;) {
    SendString("\r\n%s : ", label1);
    if(GetSecretString(maxlen, NULL)) {
      return 1;
    }
    if(inmat[0] == '\0') {
      return 0;
    }
    strcpy(tmpStr, inmat);
    SendString("\r\n%s : ", label2);
    if(GetSecretString(maxlen, NULL)) {
      return 1;
    }
    if(strcmp(inmat, tmpStr) == 0) {
      if(Servermem->cfg.cfgflags & NICFG_CRYPTEDPASSWORDS) {
        CryptPassword(inmat, pwd);
      } else {
        strncpy(pwd, inmat, maxlen + 1);
      }
      return 0;
    }
    SendString("\r\nLösenordet matchar ej.");
  }
}

int MaybeEditNumber(char *label, int *number, int maxlen, int minVal, int maxVal) {
  int tmpVal;
  for(;;) {
    SendString("\r\n%s : (%d) ", label, *number);
    if(GetStringX(EKO, maxlen, NULL, digitCharactersAccepted, NULL, NULL)) {
      return 1;
    }
    if(inmat[0]) {
      tmpVal = atoi(inmat);
      if(tmpVal < minVal || tmpVal > maxVal) {
        SendString("\r\nVärdet måste vara mellan %d och %d.", minVal, maxVal);
        continue;
      }
      *number = tmpVal;
    }
    return 0;
  }
}

int MaybeEditNumberChar(char *label, char *number, int maxlen, int minVal,
                        int maxVal) {
  int tmp = *number, ret;
  ret = MaybeEditNumber(label, &tmp, maxlen, minVal, maxVal);
  *number = tmp;
  return ret;
}

int EditBitFlag(char *label, char yesChar, char noChar, char *yesStr, char *noStr,
                long *value, long bitmask) {
  int setFlag;
  if(GetYesOrNo(label, yesChar, noChar, yesStr, noStr, *value & bitmask, &setFlag)) {
    return 1;
  }
  if(setFlag) {
    *value |= bitmask;
  } else {
    *value &= ~bitmask;
  }
  return 0;
}

int EditBitFlagShort(char *label, char yesChar, char noChar,
                     char *yesStr, char *noStr, short *value, long bitmask) {
  long tmpValue = *value, ret;
  ret = EditBitFlag(label, yesChar, noChar, yesStr, noStr, &tmpValue, bitmask);
  *value = tmpValue;
  return ret;
}

int EditBitFlagChar(char *label, char yesChar, char noChar,
                    char *yesStr, char *noStr, char *value, long bitmask) {
  long tmpValue = *value, ret;
  ret = EditBitFlag(label, yesChar, noChar, yesStr, noStr, &tmpValue, bitmask);
  *value = tmpValue;
  return ret;
}
