#include <string.h>
#include <time.h>
#include <stdlib.h>

#include <exec/types.h>
#include <utility/tagitem.h>

#include "NiKomStr.h"
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"
#include "FCrypt.h"

#define SALTLENGTH 2

char *getcryptkey(void);

/****** nikom.library/CheckPassword ******************************************

    NAME
        CheckPassword -- Checks a given password

    SYNOPSIS
        success = CheckPassword(clearText, correctPassword)
        D0                      A0         A1
        int CheckPassword(char *, char *)

    FUNCTION
        Checks if the given clear text password matched the given
        correct password. If the config flag NICFG_CRYPTEDPASSWORDS
        is true the correct password is assumed to be encrypted. If
        not a simple string comparison is performed between the two
        strings.
    	
    RESULT
        Returns non-zero if the passwords match, non zero otherwise.

    INPUTS
    	clearText - The clear text password that should be checked.
        correctPassword - The correct password to compare against.
        
*******************************************************************************/

int __saveds AASM LIBCheckPassword(
  register __a0 char *clearText AREG(a0),
  register __a1 char *correctPassword AREG(a1),
  register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

  char salt[SALTLENGTH+1], cryptbuf[14];

  if(NiKomBase->Servermem->cfg.cfgflags & NICFG_CRYPTEDPASSWORDS) {
    strncpy(salt, correctPassword, SALTLENGTH);
    salt[SALTLENGTH] = 0;
    des_fcrypt(clearText, salt, cryptbuf);
    return strcmp(cryptbuf, correctPassword) == 0 ? TRUE : FALSE;
  } else {
    return strcmp(clearText, correctPassword) == 0 ? TRUE : FALSE;
  }
}

char *__saveds AASM LIBCryptPassword(
  register __a0 char *clearText AREG(a0),
  register __a1 char *resultBuf AREG(a1),
  register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  char salt[SALTLENGTH+1];
	
  generateSalt(salt, SALTLENGTH);
  des_fcrypt(clearText, salt, resultBuf);
  return resultBuf;
}
