/*
 * Terminal.c - har hand om gränssnittet mellan basen och användaren.
 */

#include <exec/types.h>
#include <string.h>
#include <limits.h>
#include "NiKomLib.h"
#include "NiKomBase.h"
#include "Funcs.h"

/* Namn: ConvChrsToAmiga
*  Parametrar: a0 - En pekare till strängen som ska konverteras
*              d0 - Längden på strängen som ska konverteras. Vid 0
*                   konverteras det till noll-byte.
*              d1 - Från vilken teckenuppsättning ska det konverteras,
*                   definierat i NiKomLib.h
*
*  Returvärden: Inga
*
*  Beskrivning: Konverterar den inmatade strängen till ISO 8859-1 från den
*               teckenuppsättning som angivits. De tecken som inte finns i
*               ISO 8859-1 ersätts med ett '?'.
*               För CHRS_CP437 och CHRS_MAC8 konverteras bara tecken högre
*               än 128. För CHRS_SIS7 konverteras bara tecken högre än 32.
*               Tecken över 128 i SIS-strängar tolkas som ISO 8859-1.
*               CHRS_LATIN1 konverteras inte alls.
*/

void __saveds AASM LIBConvChrsToAmiga(register __a0 char *str AREG(a0), register __d0 int len AREG(d0),
	register __d1 int chrs AREG(d1), register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

	int x;
	if(chrs==CHRS_LATIN1) return;
	if(!len) len = INT_MAX;
	for(x=0;str[x] && x<len;x++) {
		switch(chrs) {
			case CHRS_CP437 :
				if(str[x]>=128) str[x] = NiKomBase->IbmToAmiga[str[x]];
				break;
			case CHRS_SIS7 :
				if(str[x]>=32) str[x] = NiKomBase->SF7ToAmiga[str[x]];
				break;
			case CHRS_MAC :
				if(str[x]>=128) str[x] = NiKomBase->MacToAmiga[str[x]];
				break;
			default :
				str[x] = convnokludge(str[x]);
		}
	}
}

/* Namn: ConvChrsFromAmiga
*  Parametrar: a0 - En pekare till strängen som ska konverteras
*              d0 - Längden på strängen som ska konverteras. Vid 0
*                   konverteras det till noll-byte.
*              d1 - Till vilken teckenuppsättning ska det konverteras,
*                   definierat i NiKomLib.h
*
*  Returvärden: Inga
*
*  Beskrivning: Konverterar den inmatade strängen från ISO 8859-1 till den
*               teckenuppsättning som angivits. De tecken som inte finns i
*               destinationsteckenuppsättningen ersätts med ett '?'.
*               För CHRS_CP437 och CHRS_MAC8 konverteras bara tecken högre
*               än 128. För CHRS_SIS7 konverteras bara tecken högre än 32.
*               CHRS_LATIN1 konverteras inte alls.
*/

void __saveds AASM LIBConvChrsFromAmiga(register __a0 char *str AREG(a0), register __d0 int len AREG(d0),
	register __d1 int chrs AREG(d1), register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

	int x;
	if(chrs==CHRS_LATIN1) return;
	if(!len) len = INT_MAX;
	for(x=0;str[x] && x<len;x++) {
		switch(chrs) {
			case CHRS_CP437 :
				if(str[x]>=128) str[x] = NiKomBase->AmigaToIbm[str[x]];
				break;
			case CHRS_SIS7 :
				if(str[x]>=32) str[x] = NiKomBase->AmigaToSF7[str[x]];
				break;
			case CHRS_MAC :
				if(str[x]>=128) str[x] = NiKomBase->AmigaToMac[str[x]];
				break;
		}
	}
}

UBYTE convnokludge(UBYTE tkn) {
	UBYTE rettkn;
	switch(tkn) {
		case 0x86: rettkn='å'; break;
		case 0x84: rettkn='ä'; break;
		case 0x94: rettkn='ö'; break;
		case 0x8F: rettkn='Å'; break;
		case 0x8E: rettkn='Ä'; break;
		case 0x99: rettkn='Ö'; break;
		case 0x91: rettkn='æ'; break;
		case 0x9b: rettkn='ø'; break;
		case 0x92: rettkn='Æ'; break;
		case 0x9d: rettkn='Ø'; break;
		default : rettkn=tkn;
	}
	return(rettkn);
}

/*	namn:		noansi()

	argument:	pekare till textsträng

	gör:		Strippar bor ANSI-sekvenser ur en textsträng.

*/

void __saveds AASM LIBStripAnsiSequences(register __a0 char *ansistr AREG(a0), register __a6 struct NiKomBase *NiKomBase AREG(a6)) {

        char *strptr, *tempstr=NULL, *orgstr;
        int index=0, status=0;


        while( (strptr=strchr(ansistr,'\x1b'))!=NULL)
        {
                orgstr = ansistr;
                index++;
                if(strptr[index]=='[')
                {
                        index++;
                        while( ((strptr[index]>='0' && strptr[index]<='9')
                                        || strptr[index]==';')
                                && strptr[index]!=NULL)
                        {
                                index++;
                                if(strptr[index]==';')
                                {
                                        tempstr=strptr;
                                        strptr=&strptr[index];
                                        index=1;
                                }
                        }
                }
                if(strptr[index]==NULL)
                        return;
                if(strptr[index]=='m')
                {
                        if(tempstr!=NULL)
                                memmove(tempstr,&strptr[index+1],strlen(&strptr[index])+1);
                        else
                                memmove(strptr,&strptr[index+1],strlen(&strptr[index])+1);
                        ansistr=orgstr;
                }
                else
                        ansistr=&strptr[index];
                index=status=0;
                tempstr=NULL;
        }
}

/* Lookup table for length of UTF-8 characters. */
static const char utf8_extra[64] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
  3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};

static int parseUTF8(char *dst, const char *src, const int si, const int len) {
  unsigned chr = (unsigned char)src[si];
  int need;

  if((chr & 0xc0) != 0xc0) {
    /* Unexpected continuation byte. */
    *dst = '?';
    return 0;
  }
  need = utf8_extra[chr - 192];
  if(len == INT_MAX) {
    unsigned idx = si+1;
    switch(need) {
    case 5:
      if(src[idx] == '\0') {
        return -5;
      }
      ++idx;
    case 4:
      if(src[idx] == '\0') {

        return -4;
      }
      ++idx;
    case 3:
      if(src[idx] == '\0') {
        return -3;
      }
      ++idx;
    case 2:
      if(src[idx] == '\0') {
        return -2;
      }
      ++idx;
    case 1:
      if(src[idx] == '\0') {
        return -1;
      }
      break;
    }
  } else if(need + 1 > (len - si)) {
    return -(need + 1 - (len - si));
  }

  switch (need) {
  case 1:
    chr &= 0x1f;
    chr <<= 6;
    chr |= src[si + 1] & 0x3f;
    if (chr < 256) {
      *dst = chr;
    } else {
      *dst = '?'; // Non ISO-8859-1
    }
    return 1;
  default:
    *dst = '?'; // Non ISO-8859-1
    break;
  }
  return need;
}

static int noConvCopy(char *dst, const char *src, unsigned len){
  unsigned i;

  for(i=0; src[i] && i < len; i++) {
    dst[i] = src[i];
  }
  return (int)i;
}

static int conv128Table(char *dst, const char *src, unsigned len,
                        const UBYTE conv[256]){
  unsigned i;

  for(i=0; src[i] && i < len; i++) {
    if(src[i] >= 128) {
      dst[i] = conv[src[i]];
    } else {
      dst[i] = src[i];
    }
  }
  return (int)i;
}

static int conv32Table(char *dst, const char *src, unsigned len,
                       const UBYTE conv[256]){
  unsigned i;

  for(i=0; src[i] && i < len; i++) {
    if(src[i] >= 32) {
      dst[i] = conv[src[i]];
    } else {
      dst[i] = src[i];
    }
  }
  return (int)i;
}

static int convNoKludgeToAmiga(char *dst, const char *src, unsigned len){
  unsigned i;

  for(i=0; src[i] && i < len; i++) {
    dst[i] = convnokludge(src[i]);
  }
  return (int)i;
}

static int convUTF8ToAmiga(char *dst, const char *src, unsigned len){
  unsigned si, di;

  for(si=0, di=0; src[si] && si < len; si++, di++) {
    if((src[si] & 0x80)) {
      int ret;
      ret = parseUTF8(dst + di, src, si, len);
      if (ret < 0) {
        return ret;
      }
      si += ret;
    } else {
      dst[di] = src[si];
    }
  }
  return (int)di;
}

/* Name: ConvMBChrsToAmiga
*  Parameters: a0 - Pointer to result string. Must be at least as big
*                   as source string.
*              a1 - Pointer to string that should be converted.
*              d0 - Length of string to convert. If 0, convert until
*                   nul.
*              d1 - Source character set, defined in NiKomLib.h
*
*  Return value: Length of new string, or negative if more bytes are
*                needed to perform conversion.
*
*  Description: Converts source string to ISO-8859-1 from the
*               specified character set. Characters not present in
*               ISO-8859-1 are replaced by '?'. For CHRS_CP437 and
*               CHRS_MAC8 only characters greater than 128 are
*               converted. For CHRS_SIS7 only characters greater than
*               32 are converted.  Character greater than 128 in
*               SIS-strings are interpreted as ISO-8859-1.
*               CHRS_LATIN1 is not converted.
*
*               Note that the destination string is not nul terminated
*               even if len is passed as zero. That must be done by
*               the caller if needed.
*/

int __saveds AASM LIBConvMBChrsToAmiga(register __a0 char *dst AREG(a0),
                                        register __a1 char *src AREG(a1),
                                        register __d0 int len AREG(d0),
                                        register __d1 int chrs AREG(d1),
                                        register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  if(len == 0) {
    len = INT_MAX;
  }
  switch(chrs) {
  case CHRS_CP437:
    return conv128Table(dst, src, len, NiKomBase->IbmToAmiga);
  case CHRS_SIS7:
    return conv32Table(dst, src, len, NiKomBase->SF7ToAmiga);
  case CHRS_MAC:
    return conv128Table(dst, src, len, NiKomBase->MacToAmiga);
  case CHRS_UTF8:
    return convUTF8ToAmiga(dst, src, len);
  case CHRS_LATIN1:
    return noConvCopy(dst, src, len);
  default:
    return convNoKludgeToAmiga(dst, src, len);
  }
  /* Not reached. */
}

static int convUTF8FromAmiga(char *dst, const char *src, unsigned len){
  unsigned si, di;

  for(si=0, di=0; src[si] && si < len; si++, di++) {
    if((src[si] & 0x80)) {
      unsigned char c = src[si];
      dst[di] = 0xc0 | ((unsigned) c >> 6);
      di++;
      dst[di] = 0x80 | (c & 0x3f);
      continue;
    } else {
      dst[di] = src[si];
    }
  }
  return (int)di;
}

/* Name: ConvMBChrsFromAmiga
*  Parameters: a0 - Pointer to result string. Must be at least twice
*                   as big as source string.
*              a1 - Pointer to string that should be converted.
*              d0 - Length of string to convert. If 0, convert until
*                   nul.
*              d1 - Destination character set, defined in NiKomLib.h
*
*  Return value: Length of new string.
*
*  Description: Converts source string from ISO-8859-1 from the
*               specified character set. Characters not present in the
*               destination character set are replaced by '?'.  For
*               CHRS_CP437 and CHRS_MAC8 only characters greater than
*               128 are converted. For CHRS_SIS7 only characters
*               greater than 32 are converted. For UTF-8 the
*               destination string may be upto twice as long as the
*               source string.
*               CHRS_LATIN1 is not converted.
*
*               Note that the destination string is not nul terminated
*               even if len is passed as zero. That must be done by
*               the caller if needed.
*/

int __saveds AASM LIBConvMBChrsFromAmiga(register __a0 char *dst AREG(a0),
                                          register __a1 char *src AREG(a1),
                                          register __d0 int len AREG(d0),
                                          register __d1 int chrs AREG(d1),
                                          register __a6 struct NiKomBase *NiKomBase AREG(a6)) {
  if(len == 0) {
    len = INT_MAX;
  }
  switch(chrs) {
  case CHRS_CP437:
    return conv128Table(dst, src, len, NiKomBase->AmigaToIbm);
  case CHRS_SIS7:
    return conv32Table(dst, src, len, NiKomBase->AmigaToSF7);
  case CHRS_MAC:
    return conv128Table(dst, src, len, NiKomBase->AmigaToMac);
  case CHRS_UTF8:
    return convUTF8FromAmiga(dst, src, len);
  case CHRS_LATIN1:
  default:
    return noConvCopy(dst, src, len);
  }
  /* Not reached. */
}
